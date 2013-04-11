#include "spotify-fs.h"
#include <string.h>
#include <errno.h>
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
