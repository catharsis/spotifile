#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "spotify-fs.h"

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
