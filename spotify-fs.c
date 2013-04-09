#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "spotify-fs.h"

int spfs_getattr(const char *path, struct stat *statbuf)
{
	int res = 0;
	memset(statbuf, 0, sizeof(struct stat));
	if(strcmp(path, "/") == 0) {
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_nlink = 2;
	}
	else if (strcmp(path, "/connection") == 0) {
		statbuf->st_mode = S_IFREG | 0444;
		statbuf->st_nlink = 1;
		statbuf->st_size = 64;
	}
	else
		res = -ENOENT;
	return res;
}

size_t connection_file_read(char *buf, size_t size, off_t offset) {
	sp_connectionstate connectionstate = sp_session_connectionstate(spotify_session);
	char *state_str;
	size_t len = 0;
	switch (connectionstate) {
		case SP_CONNECTION_STATE_LOGGED_OUT: //User not yet logged in.
			state_str = strdup("logged out");
			break;
		case SP_CONNECTION_STATE_LOGGED_IN: //Logged in against a Spotify access point.
			state_str = strdup("logged in");
			break;
		case SP_CONNECTION_STATE_DISCONNECTED: //Was logged in, but has now been disconnected.
			state_str = strdup("disconnected");
			break;
		case SP_CONNECTION_STATE_OFFLINE:
			state_str = strdup("offline");
			break;
		case SP_CONNECTION_STATE_UNDEFINED: //The connection state is undefined.
			//FALLTHROUGH
		default:
			state_str = strdup("undefined");
			break;
	}

	if ((len = strlen(state_str)) > offset) {
		if ( offset + size > len)
			size = len - offset;
		memcpy(buf, state_str + offset, size);
	}
	else
		size = 0;

	return size;
}

int spfs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	if (strcmp(path, "/connection") != 0)
		return -ENOENT;

	return connection_file_read(buf, size, offset);
}

int spfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, "connection", NULL, 0);
	return 0;
}
struct fuse_operations spfs_operations = {
  .getattr = spfs_getattr,
  .read = spfs_read,
  .readdir = spfs_readdir,
};


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
int main(int argc, char *argv[])
{
	int retstat = 0;
	char *password = NULL;
	char *username = malloc(SPOTIFY_USERNAME_MAXLEN);
	if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running spotifile as root is not a good idea\n");
		return 1;
    }
	printf("spotify username: ");
	username = fgets(username, SPOTIFY_USERNAME_MAXLEN, stdin);
	password = getpass("spotify password: ");
	if (strlen(password) <= 0)
	{
		password = NULL;
	}

    fprintf(stderr, "about to call fuse_main\n");
	if(spotify_session_init(username, password, NULL) == 0)
		retstat = fuse_main(argc, argv, &spfs_operations, NULL);

	return retstat;
}
