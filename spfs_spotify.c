#include "spotify-fs.h"
#include <string.h>
#include <stdio.h>
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

	config.user_agent = "spotifile";
	error = sp_session_create(&config, &session);
	if ( error != SP_ERROR_OK ) {
		fprintf(stderr, "failed to create session: %s\n",
				sp_error_message(error));
		return 2;
	}
	printf("session created!\n");
	if(spotify_login(session, username, password, blob) == 0) {
		spotify_session = session;
		return 0;
	}
	else {
		fprintf(stderr, "login failed!\n");
		return 1;
	}

}
