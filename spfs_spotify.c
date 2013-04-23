#include "spotify-fs.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
sp_session *g_spotify_session;
time_t g_logged_in_at = (time_t) -1;
pthread_mutex_t spotify_mutex;
pthread_t spotify_thread;

/*foward declarations*/
void * spotify_thread_start_routine(void *arg);

int spotify_login(sp_session *session, const char *username, const char *password, const char *blob) {
	if (username == NULL) {
		char reloginname[256];

		if (sp_session_relogin(session) == SP_ERROR_NO_CREDENTIALS) {
			spfs_log("no stored credentials");
			return 1;
		}
		sp_session_remembered_user(session, reloginname, sizeof(reloginname));
		spfs_log("Trying to relogin as user %s\n", reloginname);

	} else {
		spfs_log("Trying to login as %s", username);
		sp_session_login(session, username, password, 1, blob);
	}

	return 0;

}
static void spotify_log_message(sp_session *session, const char *message) {
	spfs_log("spotify: %s", message);
}

static void spotify_logged_out(sp_session *session) {
	g_logged_in_at = (time_t) -1;
	spfs_log("spotify login: logged out");
}

static void spotify_logged_in(sp_session *session, sp_error error)
{
	/* TODO: might be we want to keep this information available
	 * to users through some file so that they don't have to sift
	 * through the logs?*/
	spfs_log("spotify login: %s", sp_error_message(error));
	if(SP_ERROR_OK == error) {
		time(&g_logged_in_at);
		spfs_log("logged in successfully at %d", g_logged_in_at);
	}
}

static void spotify_connection_error(sp_session *session, sp_error error)
{
	spfs_log("Connection error %d: %s\n", error, sp_error_message(error));
}

static sp_session_callbacks spotify_callbacks = {
	.logged_in = spotify_logged_in,
	.connection_error = spotify_connection_error,
	.logged_out = spotify_logged_out,
	.log_message = spotify_log_message,
};

int spotify_session_init(const char *username, const char *password,
		const char *blob)
{
	int s = 0;
	extern const uint8_t g_appkey[];
	extern const size_t g_appkey_size;
	sp_session_config config;
	sp_error error;
	sp_session *session;
	memset(&config, 0, sizeof(config));
	config.api_version = SPOTIFY_API_VERSION;
	config.cache_location = "tmp";
	config.settings_location = "tmp";
	config.application_key = g_appkey;
	config.application_key_size = g_appkey_size;
	config.callbacks = &spotify_callbacks;

	config.user_agent = application_name;
	error = sp_session_create(&config, &session);
	if ( error != SP_ERROR_OK ) {
		spfs_log("failed to create session: %s",
				sp_error_message(error));
		return 2;
	}
	spfs_log("spotify session created!");
	if(spotify_login(session, username, password, blob) == 0) {
		g_spotify_session = session;
	}
	else {
		spfs_log("login failed!");
		return 1;
	}

	s = pthread_mutex_init(&spotify_mutex, NULL);
	if ( s != 0) {
		handle_error_en(s, "pthread_mutex_init");
	}

	s = pthread_create(&spotify_thread, NULL,
			spotify_thread_start_routine, (void *)NULL);
	if ( s != 0) {
		handle_error_en(s, "pthread_create");
	}
	return 0;
}


/* locking accessors */
sp_connectionstate spotify_connectionstate() {
	int ret = 0;
	sp_connectionstate s;

	MUTEX_LOCK(ret, &spotify_mutex);
	s = sp_session_connectionstate(g_spotify_session);
	MUTEX_UNLOCK(ret, &spotify_mutex);

	return s;
}

/* "public" convenience functions */
char * spotify_connectionstate_str() {
	sp_connectionstate connectionstate = spotify_connectionstate();
	char *str = NULL;
	switch (connectionstate) {
		case SP_CONNECTION_STATE_LOGGED_OUT:
			str = strdup("logged out");
			break;
		case SP_CONNECTION_STATE_LOGGED_IN:
			str = strdup("logged in");
			break;
		case SP_CONNECTION_STATE_DISCONNECTED:
			str = strdup("disconnected");
			break;
		case SP_CONNECTION_STATE_OFFLINE:
			str = strdup("offline");
			break;
		case SP_CONNECTION_STATE_UNDEFINED: /* FALLTHROUGH */
		default:
			str = strdup("undefined");
			break;
	}
	if (!str)
		handle_error_en(ENOMEM, "strdup");

	return str;
}
/*thread routine*/
void * spotify_thread_start_routine(void *arg) {
	int event_timeout = 0, ret = 0;
	sp_error err;
	spfs_log("spotify session processing thread started");
	for(;;) {
		MUTEX_LOCK(ret, &spotify_mutex);
		err = sp_session_process_events(g_spotify_session, &event_timeout);
		MUTEX_UNLOCK(ret, &spotify_mutex);
		if (err != SP_ERROR_OK) {
			spfs_log("Could not process events (%d): %s\n", err, sp_error_message(err));
		}
		usleep(event_timeout);
	}
	spfs_log("spotify session processing thread ended");
	return (void *)NULL;
}
