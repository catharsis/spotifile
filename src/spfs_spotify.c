#include "spotify-fs.h"
#include "spfs_spotify.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
time_t g_logged_in_at = (time_t) -1;

/* thread globals */
static bool g_main_thread_do_notify = false;
static bool g_running = false;
static GMutex g_spotify_notify_mutex;
static GMutex g_spotify_api_mutex;
static GCond g_spotify_notify_cond;
static GCond g_spotify_data_available;
static GThread *spotify_thread;

/*foward declarations*/
void * spotify_thread_start(void *arg);

int spotify_login(sp_session * session, const char *username, const char *password, const char *blob) {
	if (username == NULL) {
		char reloginname[256];

		if (sp_session_relogin(session) == SP_ERROR_NO_CREDENTIALS) {
			g_info("no stored credentials");
			return 1;
		}

		sp_session_remembered_user(session, reloginname, sizeof(reloginname));
		g_info("trying to relogin as user %s", reloginname);

	} else {
		g_info("trying to login as %s", username);
		sp_session_login(session, username, password, 1, blob);
	}
	return 0;
}

static void spotify_log_message(sp_session *session, const char *message) {
	g_info("spotify: %s", message);
}

static void spotify_logged_out(sp_session *session) {
	g_logged_in_at = (time_t) -1;
	g_info("spotify: logged out");
}

static void spotify_logged_in(sp_session *session, sp_error error)
{
	if(SP_ERROR_OK == error) {
		time(&g_logged_in_at);
		g_info("spotify: logged in at %d", g_logged_in_at);
	} else {
		g_info("spotify: logged in failed (%s)", sp_error_message(error));
	}
}

static void spotify_connection_error(sp_session *session, sp_error error)
{
	g_warning("Connection error %d: %s\n", error, sp_error_message(error));
}

static void artist_search_complete_cb(sp_search *result, void *userdata)
{
	int ret = 0;
	g_debug("Artist search complete!");
	g_mutex_lock(&g_spotify_api_mutex);
	g_cond_broadcast(&g_spotify_data_available);
	g_mutex_unlock(&g_spotify_api_mutex);
}

static void spotify_notify_main_thread(sp_session *session)
{
	int ret = 0;
	g_mutex_lock(&g_spotify_notify_mutex);
	g_main_thread_do_notify = true;
	g_cond_signal(&g_spotify_notify_cond);
	g_mutex_unlock(&g_spotify_notify_mutex);

}

static sp_session_callbacks spotify_callbacks = {
	.notify_main_thread = spotify_notify_main_thread,
	.logged_in = spotify_logged_in,
	.connection_error = spotify_connection_error,
	.logged_out = spotify_logged_out,
	.log_message = spotify_log_message,
};

sp_session * spotify_session_init(const char *username, const char *password, const char *blob)
{
	extern const uint8_t g_appkey[];
	extern const size_t g_appkey_size;
	sp_session_config config;
	sp_error error;
	sp_session *session;
	memset(&config, 0, sizeof(config));
	config.api_version = SPOTIFY_API_VERSION;
	config.cache_location = "/var/tmp/";
	config.settings_location = "/tmp";
	config.application_key = g_appkey;
	config.application_key_size = g_appkey_size;
	config.callbacks = &spotify_callbacks;

	config.user_agent = application_name;
	error = sp_session_create(&config, &session);
	if ( error != SP_ERROR_OK ) {
		g_warning("failed to create session: %s",
				sp_error_message(error));
		return NULL;
	}
	g_info("spotify session created!");
	if(spotify_login(session, username, password, blob) != 0)
		g_warning("login failed!");

	return session;
}

void spotify_session_destroy(sp_session * session)
{
	sp_session_logout(session);
	free(session);
	g_info("session destroyed");
}

void spotify_threads_init(sp_session *session)
{
	g_mutex_init(&g_spotify_api_mutex);
	g_mutex_init(&g_spotify_notify_mutex);
	g_cond_init(&g_spotify_notify_cond);
	g_cond_init(&g_spotify_data_available);
	g_running = true;
	spotify_thread = g_thread_new("spotify", spotify_thread_start, session);
}

void spotify_threads_destroy()
{
	int s = 0;
	g_running = false;
	g_debug("spotify thread cancel request sent");
	g_thread_join(spotify_thread);	
	spotify_thread = NULL;
	g_debug("spotify threads destroyed");
}


/* locking accessors */
sp_connectionstate spotify_connectionstate(sp_session * session) {
	int ret = 0;
	sp_connectionstate s;

	g_mutex_lock(&g_spotify_api_mutex);
	s = sp_session_connectionstate(session);
	g_mutex_unlock(&g_spotify_api_mutex);

	return s;
}

/* "public" convenience functions */
const char * spotify_connectionstate_str(sp_connectionstate connectionstate) {
	switch (connectionstate) {
		case SP_CONNECTION_STATE_LOGGED_OUT:
			return "logged out";
			break;
		case SP_CONNECTION_STATE_LOGGED_IN:
			return "logged in";
			break;
		case SP_CONNECTION_STATE_DISCONNECTED:
			return "disconnected";
			break;
		case SP_CONNECTION_STATE_OFFLINE:
			return "offline";
			break;
		case SP_CONNECTION_STATE_UNDEFINED: /* FALLTHROUGH */
		default:
			return "undefined";
			break;
	}
}

GSList *spotify_artist_search(sp_session * session, const char *query) {
	int ret = 0, i = 0, num_artists = 0;
	sp_search *search = NULL;
	sp_artist *artist = NULL;
	GSList *artists = NULL;
	g_return_val_if_fail(session != NULL, NULL);
	g_return_val_if_fail(query != NULL, NULL);

	sp_connectionstate connstate = spotify_connectionstate(session);
	if ( connstate != SP_CONNECTION_STATE_LOGGED_IN) {
		g_debug("Not logged in (%s), won't search for artist %s", spotify_connectionstate_str(connstate), query);
		return NULL;
	}
	g_mutex_lock(&g_spotify_api_mutex);
	search = sp_search_create(session, query, 0, 0, 0, 0, 0, 100, 0, 0, SP_SEARCH_STANDARD, artist_search_complete_cb, NULL);
	g_debug("search created, waiting on load");
	
	gint64 end_time = g_get_monotonic_time() + 30 * G_TIME_SPAN_SECOND;
	while (!sp_search_is_loaded(search)) {
		if (!g_cond_wait_until(&g_spotify_data_available, &g_spotify_api_mutex, end_time))
		{
			g_mutex_unlock(&g_spotify_api_mutex);
			g_debug("still not loaded...giving up");
			return NULL;
		}
	}
	
	num_artists = sp_search_num_artists(search);
	g_debug("Found %d artists", num_artists);
	if (num_artists > 0) {
		for (i = 0; i < num_artists; i++) {
			artist = sp_search_artist(search, i);
			artists = g_slist_append(artists, artist); /*sp_artist_ref?*/
			g_debug("Found artist: %s", sp_artist_name(artist));
		}
	}
	sp_search_release(search);
	g_mutex_unlock(&g_spotify_api_mutex);
	return artists;
}

const gchar * spotify_artist_name(sp_artist *artist) {
	const gchar *name = NULL;
	g_mutex_lock(&g_spotify_api_mutex);
	name = sp_artist_name(artist);
	g_mutex_unlock(&g_spotify_api_mutex);
	return name;
}

sp_link * spotify_link_create_from_artist(sp_artist *artist) {
	sp_link *link = NULL;
	g_mutex_lock(&g_spotify_api_mutex);
	link = sp_link_create_from_artist(artist);
	g_mutex_unlock(&g_spotify_api_mutex);
	return link;
}

int spotify_link_as_string(sp_link *link, char *buf, int buffer_size) {
	int ret = 0;
	g_mutex_lock(&g_spotify_api_mutex);
	ret = sp_link_as_string(link, buf, buffer_size);
	g_mutex_unlock(&g_spotify_api_mutex);
	return ret;

}

/*thread routine*/
void * spotify_thread_start(void *arg) {
	int event_timeout = 0, ret = 0;
	sp_error err;
	sp_session * session = (sp_session *) arg;
	g_return_val_if_fail(session != NULL, NULL);
	g_debug("spotify session processing thread: started");
	g_mutex_lock(&g_spotify_notify_mutex);
	while (g_running) {
		if (event_timeout == 0) {
			while (!g_main_thread_do_notify) {
				g_debug("spotify session processing thread: waiting on notify mutex");
				g_cond_wait(&g_spotify_notify_cond, &g_spotify_notify_mutex);
			}
		}
		else {	
			gint64 end_time = g_get_monotonic_time () + event_timeout;
			g_cond_wait_until(&g_spotify_notify_cond, &g_spotify_notify_mutex, end_time);
		}
		g_main_thread_do_notify = false;

		g_mutex_unlock(&g_spotify_notify_mutex);

		do {
			err = sp_session_process_events(session, &event_timeout);
			if (err != SP_ERROR_OK) {
				g_warning("spotify session processing thread: Could not process events (%d): %s\n", err, sp_error_message(err));
			}
		} while (event_timeout == 0);
		g_mutex_lock(&g_spotify_notify_mutex);
	}
	g_debug("spotify session processing thread: ended");
	return (void *)NULL;
}
