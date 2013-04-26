#include "spotify-fs.h"
#include <string.h>
#include <errno.h>
extern time_t g_logged_in_at;

size_t connection_file_read(char *buf, size_t size, off_t offset);
void connection_file_getattr(struct stat *statbuf);

struct spfs_file {
	char *abs_path;
	void (*spfs_file_getattr)(struct stat *);
	size_t (*spfs_file_read)(char *, size_t, off_t);
};

static struct spfs_file *spfs_files[] = {
	&(struct spfs_file){ /*connection file*/
		.abs_path = "/connection",
		.spfs_file_getattr = connection_file_getattr,
		.spfs_file_read = connection_file_read
	},
	/*SENTINEL*/
	NULL,
};

int spfs_getattr(const char *path, struct stat *statbuf)
{
	int i = 0;
	struct spfs_file *tmpfile = NULL;
	memset(statbuf, 0, sizeof(struct stat));
	if(strcmp(path, "/") == 0) {
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_nlink = 2;
		return 0;
	}
	else {
		while ((tmpfile = spfs_files[i++])) {
			if (strcmp(tmpfile->abs_path, path) == 0) {
				tmpfile->spfs_file_getattr(statbuf);
				return 0;
			}
		}
	}
	return -ENOENT;
}

void connection_file_getattr(struct stat *statbuf)
{
	statbuf->st_mode = S_IFREG | 0444;
	statbuf->st_nlink = 1;
	statbuf->st_size = 64;
	statbuf->st_mtime = g_logged_in_at;
}

size_t connection_file_read(char *buf, size_t size, off_t offset) {
	char *state_str = spotify_connectionstate_str();
	size_t len = 0;

	if ((len = strlen(state_str)) > offset) {
		if ( offset + size > len)
			size = len - offset;
		memcpy(buf, state_str + offset, size);
	}
	else
		size = 0;

	free(state_str);
	return size;
}

int spfs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	int i = 0;
	struct spfs_file *tmpfile;
	while((tmpfile = spfs_files[i++]) != NULL)
	{
		if (strcmp(tmpfile->abs_path, path) == 0)
			return tmpfile->spfs_file_read(buf, size, offset);
	}
	return -ENOENT;

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
