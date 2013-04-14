#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "spotify-fs.h"

struct spotify_credentials {
	char *username;
	char *password;
};


/*real stupid logging function, for now*/
void spfs_log(const char *message)
{
	FILE *logfile = fopen("spotifile.log", "a");
	fprintf(logfile, message);
	fclose(logfile);
}
void *spfs_init(struct fuse_conn_info *conn)
{
	struct fuse_context *context = fuse_get_context();
	struct spotify_credentials *credentials = (struct spotify_credentials *)context->private_data;
	/* should we do something about this, really?
	 * Maybe put error logging here instead of in
	 * spotify_session_init()*/
	(void) spotify_session_init(credentials->username, credentials->password, NULL);
	return NULL;

}

struct spotify_credentials *request_credentials()
{
	struct spotify_credentials *credentials = malloc(sizeof(struct spotify_credentials));
	credentials->username = malloc(SPOTIFY_USERNAME_MAXLEN);

	printf("spotify username: ");
	credentials->username = fgets(credentials->username, SPOTIFY_USERNAME_MAXLEN, stdin);
	if (credentials->username != NULL) {
		long username_len = strlen(credentials->username);
		if(username_len > 0 && credentials->username[username_len-1] == '\n') {
			credentials->username[username_len-1] = '\0';
		}
	}

	credentials->password = getpass("spotify password: ");
	if (strlen(credentials->password) <= 0)
	{
		credentials->password = NULL;
	}
	return credentials;
}

int main(int argc, char *argv[])
{
	int retval = 0;
	struct spotify_credentials *credentials = NULL;
	if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running %s as root is not a good idea\n", application_name);
		return 1;
	}
	credentials = request_credentials();
	spfs_operations.init = spfs_init;
	retval = fuse_main(argc, argv, &spfs_operations, credentials);
	if (retval != 0) {
		fprintf(stderr, "Error initialising spotifile\n");
	}
	return retval;
}
