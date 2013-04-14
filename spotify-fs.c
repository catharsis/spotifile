#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "spotify-fs.h"

int main(int argc, char *argv[])
{
	int retval = 0;
	char *password = NULL;
	char *username = malloc(SPOTIFY_USERNAME_MAXLEN);
	if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running %s as root is not a good idea\n", application_name);
		return 1;
	}
	printf("spotify username: ");
	username = fgets(username, SPOTIFY_USERNAME_MAXLEN, stdin);
	long username_len = strlen(username);
	if(username[username_len-1] == '\n') {
		username[username_len-1] = '\0';
	}
	password = getpass("spotify password: ");
	if (strlen(password) <= 0)
	{
		password = NULL;
	}

	/* should we do something about this, really?
	 * Maybe put error logging here instead of in
	 * spotify_session_init()*/
	(void) spotify_session_init(username, password, NULL);
	retval = fuse_main(argc, argv, &spfs_operations, NULL);
	return retval;
}
