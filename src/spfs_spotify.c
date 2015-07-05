#include "spotify-fs.h"
#include "spfs_spotify.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <glib.h>
time_t g_logged_in_at = (time_t) -1;

/* thread globals */
static bool g_main_thread_do_notify = false;
static pthread_mutex_t g_spotify_notify_mutex;
static pthread_mutex_t g_spotify_api_mutex;
static pthread_cond_t g_spotify_notify_cond;
static pthread_cond_t g_spotify_data_available;
static pthread_t spotify_thread;

/*foward declarations*/
void * spotify_thread_start_routine(void *arg);

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
		g_info("logged in successfully at %d", g_logged_in_at);
	} else {
		g_info("spotify login: %s", sp_error_message(error));
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
	MUTEX_LOCK(ret, &g_spotify_api_mutex);
	pthread_cond_broadcast(&g_spotify_data_available);
	MUTEX_UNLOCK(ret, &g_spotify_api_mutex);
}

static void spotify_notify_main_thread(sp_session *session)
{
	int ret = 0;
	MUTEX_LOCK(ret, &g_spotify_notify_mutex);
	g_main_thread_do_notify = true;
	pthread_cond_signal(&g_spotify_notify_cond);
	MUTEX_UNLOCK(ret, &g_spotify_notify_mutex);

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
	config.cache_location = "/tmp";
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
	int s = 0;
	
	s = pthread_mutex_init(&g_spotify_api_mutex, NULL);
	if ( s != 0) {
		handle_error_en(s, "pthread_mutex_init");
	}
	
	s = pthread_mutex_init(&g_spotify_notify_mutex, NULL);
	if ( s != 0) {
		handle_error_en(s, "pthread_mutex_init");
	}

	s = pthread_cond_init(&g_spotify_notify_cond, NULL);
	if ( s != 0) {
		handle_error_en(s, "pthread_cond_init");
	}

	s = pthread_cond_init(&g_spotify_data_available, NULL);
	if ( s != 0) {
		handle_error_en(s, "pthread_cond_init");
	}
	s = pthread_create(&spotify_thread, NULL,
			spotify_thread_start_routine, session);
	if ( s != 0) {
		handle_error_en(s, "pthread_create");
	}
}

void spotify_threads_destroy()
{
	int s = 0;
	s = pthread_cancel(spotify_thread);
	if ( s != 0) {
		handle_error_en(s, "pthread_cancel");
	}
	g_debug("spotify thread cancel request sent");
	s = pthread_join(spotify_thread, NULL);
	if ( s != 0) {
		g_debug("failed to join spotify thread!");
		handle_error_en(s, "pthread_join");
	}
	spotify_thread = -1;
	g_debug("spotify threads destroyed");
}


/* locking accessors */
sp_connectionstate spotify_connectionstate(sp_session * session) {
	int ret = 0;
	sp_connectionstate s;

	MUTEX_LOCK(ret, &g_spotify_api_mutex);
	s = sp_session_connectionstate(session);
	MUTEX_UNLOCK(ret, &g_spotify_api_mutex);

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

char ** spotify_artist_search(sp_session * session, const char *query) {
	int ret = 0, i = 0, num_artists = 0;
	sp_search *search = NULL;
	sp_artist *artist = NULL;
	char **artists = NULL;
	g_debug("initiating query");
	if (!query) {
		return NULL;
	}
	sp_connectionstate connstate = spotify_connectionstate(session);
	if ( connstate != SP_CONNECTION_STATE_LOGGED_IN) {
		g_debug("Not logged in (%s), won't search for artist %s", spotify_connectionstate_str(connstate), query);
		return NULL;
	}
	MUTEX_LOCK(ret, &g_spotify_api_mutex);
	search = sp_search_create(session, query, 0, 0, 0, 0, 0, 100, 0, 0, SP_SEARCH_STANDARD, artist_search_complete_cb, NULL);
	g_debug("search created, waiting on load");
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 60;
	while (!sp_search_is_loaded(search)) {
		if (pthread_cond_timedwait(&g_spotify_data_available, &g_spotify_api_mutex, &ts) != 0)
		{
			g_debug("still not loaded...giving up");
			return NULL;
		}

		g_debug("still not loaded...");
	}
	
	

	num_artists = sp_search_num_artists(search);
	g_debug("Found %d artists", num_artists);
	if (num_artists > 0) {
		artists = g_malloc_n(num_artists+1, sizeof(char*));

		for (i = 0; i < num_artists; i++) {
			artist = sp_search_artist(search, i);
			artists[i] = g_strdup(sp_artist_name(artist));
			g_debug("Found artist: %s", artists[i]);
		}
		artists[i] = NULL;
	}
	sp_search_release(search);
	MUTEX_UNLOCK(ret, &g_spotify_api_mutex);
	return artists;
}

void spotify_artist_search_destroy(char **artists) {
	int i = 0;
	if (!artists) return;

	while (artists[i] != NULL) {
		g_free(artists[i]);
		artists[i] = NULL;
		i++;
	}
	g_free(artists);
	artists = NULL;
}

/*thread routine*/
void * spotify_thread_start_routine(void *arg) {
	int event_timeout = 0, ret = 0;
	sp_error err;
	sp_session * session = (sp_session *) arg;
	g_debug("spotify session processing thread: started");
	MUTEX_LOCK(ret, &g_spotify_notify_mutex);
	for(;;) {
		if (event_timeout == 0) {
			while (!g_main_thread_do_notify) {
				g_debug("spotify session processing thread: waiting on notify mutex");
				pthread_cond_wait(&g_spotify_notify_cond, &g_spotify_notify_mutex);
			}
		}
		else {
			
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);

            ts.tv_sec += event_timeout / 1000;
            ts.tv_nsec += (event_timeout % 1000) * 1000000;

            pthread_cond_timedwait(&g_spotify_notify_cond, &g_spotify_notify_mutex, &ts);
		}
		g_main_thread_do_notify = false;

		MUTEX_UNLOCK(ret, &g_spotify_notify_mutex);

		do {
			err = sp_session_process_events(session, &event_timeout);
			if (err != SP_ERROR_OK) {
				g_warning("spotify session processing thread: Could not process events (%d): %s\n", err, sp_error_message(err));
			}
		} while (event_timeout == 0);
		MUTEX_LOCK(ret, &g_spotify_notify_mutex);
	}
	g_debug("spotify session processing thread: ended");
	return (void *)NULL;
}
