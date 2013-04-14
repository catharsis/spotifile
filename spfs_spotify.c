#include "spotify-fs.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
sp_session *g_spotify_session;
pthread_mutex_t spotify_mutex;
pthread_t spotify_thread;

#define MUTEX_LOCK(mx_lock_ret, m) \
	do { \
		if ((mx_lock_ret = pthread_mutex_lock(m)) != 0) \
			handle_error_en(mx_lock_ret, "pthread_mutex_lock"); \
	} while (0)

#define MUTEX_UNLOCK(mx_unlock_ret, m) \
	do { \
		if ((mx_unlock_ret = pthread_mutex_unlock(m)) != 0) \
			handle_error_en(mx_unlock_ret, "pthread_mutex_unlock"); \
	} while (0)

int spotify_login(sp_session *session, const char *username, const char *password, const char *blob) {
	if (username == NULL) {
		printf("no credentials given, trying relogin\n");
		char reloginname[256];

		if (sp_session_relogin(session) == SP_ERROR_NO_CREDENTIALS) {
			fprintf(stderr, "no stored credentials\n");
			return 1;
		}
		sp_session_remembered_user(session, reloginname, sizeof(reloginname));
		fprintf(stderr, "Trying to relogin as user %s\n", reloginname);

	}
	else
		sp_session_login(session, username, password, 1, blob);

	return 0;

}

static void spotify_logged_in(sp_session *session, sp_error error)
{
	return;
}

static sp_session_callbacks spotify_callbacks = {
		.logged_in = spotify_logged_in,
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
		fprintf(stderr, "failed to create session: %s\n",
				sp_error_message(error));
		return 2;
	}
	printf("session created!\n");
	if(spotify_login(session, username, password, blob) == 0) {
		g_spotify_session = session;
		return 0;
	}
	else {
		fprintf(stderr, "login failed!\n");
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
	return (void *)NULL;
}
