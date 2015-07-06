#include "spotify-fs.h"
#include "spfs_spotify.h"
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <libgen.h>
#include <glib.h>
extern time_t g_logged_in_at;


#define SP_SESSION (sp_session *)(fuse_get_context()->private_data)
/* functional files - forward function declarations */
size_t connection_file_read(char *buf, size_t size, off_t offset);
bool connection_file_getattr(const char *path, struct stat *statbuf);
bool search_file_getattr(const char *path, struct stat *statbuf);

struct spfs_file {
	char *abs_path;
	bool (*spfs_file_getattr)(const char *, struct stat *);
	size_t (*spfs_file_read)(char *, size_t, off_t);
};

static struct spfs_file *spfs_files[] = {
	&(struct spfs_file){ /*connection file*/
		.abs_path = "/connection",
		.spfs_file_getattr = connection_file_getattr,
		.spfs_file_read = connection_file_read
	},
	&(struct spfs_file){ /*search*/
		.abs_path = "/search",
		.spfs_file_getattr = search_file_getattr,
		.spfs_file_read = NULL
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
			if (strncmp(tmpfile->abs_path, path, strlen(tmpfile->abs_path)) == 0) {
				if (tmpfile->spfs_file_getattr(path, statbuf)) {
					return 0;
				}
			}
		}
	}
	return -ENOENT;
}

bool connection_file_getattr(const char* path, struct stat *statbuf)
{
	if (strcmp("/connection", path) != 0) {
		return false;
	}
	else {
		statbuf->st_mode = S_IFREG | 0444;
		statbuf->st_nlink = 1;
		statbuf->st_size = 64;
		statbuf->st_mtime = g_logged_in_at;
		return true;
	}
}

bool search_file_getattr(const char *path, struct stat *statbuf)
{
	bool ret = true;
	if (g_strcmp0("/search", path) == 0) {
		/*search root*/
		statbuf->st_mtime = time(NULL);
		statbuf->st_mode = S_IFDIR | 0755;
	}
	else if (g_str_has_prefix(path, "/search/artists"))
	{
		/*artist search*/	
		char *dir = g_path_get_dirname(path);
		if (g_strcmp0(path, "/search/artists") == 0) {
			statbuf->st_mode = S_IFDIR | 0755;
			statbuf->st_mtime = time(NULL);
		}
		else if (g_strcmp0(dir, "/search/artists") == 0) {
			/* here we set up a directory, which if changed into will generate
			 * a query for artists matching the directory name.
			 * We defer the actual query, since executing it here might impact
			 * performance due to shell tab completion, and other stat'ing shell extensions*/
			statbuf->st_mode = S_IFDIR | 0755;
			statbuf->st_mtime = time(NULL);
		}
		else {
			/* This is a search result. */
			statbuf->st_mode = S_IFLNK | 0755;
			statbuf->st_mtime = time(NULL);
		}
		g_free(dir);
	}
	
	return ret;
}


size_t connection_file_read(char *buf, size_t size, off_t offset) {
	sp_session *session = SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	char *state_str =
		g_strdup(spotify_connectionstate_str(spotify_connectionstate(session)));

	size_t len = 0;

	if ((len = strlen(state_str)) > offset) {
		if ( offset + size > len)
			size = len - offset;
		memcpy(buf, state_str + offset, size);
	}
	else
		size = 0;

	g_free(state_str);
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

int spfs_readlink(const char *path, char *buf, size_t len) {
	strncpy(buf, "../..", len);
	return 0;
}

int spfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	/*TODO: remove the hardcoding of paths, parse
	 * spfs_files instead*/
	sp_session *session = SP_SESSION;
	g_debug("readdir path:%s ", path);
	g_return_val_if_fail(session != NULL, 0);
	char *dirname_path_copy = strdup(path);
	char *basename_path_copy = strdup(path);
	char *artist = NULL;
	char **artists;
	int ret = 0, i = 0;
	if (strcmp(path, "/") == 0)
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		filler(buf, "connection", NULL, 0);
		filler(buf, "search", NULL, 0);
	}
	else if (strcmp(path, "/search") == 0) {
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		filler(buf, "artists", NULL, 0);
	}
	else if (strcmp(dirname(dirname_path_copy), "/search/artists") == 0) {
		/*artist query*/
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		artist = g_strdup(basename(basename_path_copy));
		g_debug("querying for artist: %s", artist);
		artists = spotify_artist_search(session, artist);
		g_free(artist);
		if (artists != NULL) {
			while (artists[i]) {
				struct stat *st = g_new0(struct stat, 1);
				st->st_mode = S_IFLNK;
				filler(buf, artists[i], st, 0);
				++i;
			}
			spotify_artist_search_destroy(artists);
		}
		else {
			g_debug("no artist search result");
			ret = -ENOENT;
		}
	}
	else
		ret = -ENOENT;

	return ret;
}

struct fuse_operations spfs_operations = {
	.getattr = spfs_getattr,
	.read = spfs_read,
	.readdir = spfs_readdir,
	.readlink = spfs_readlink
};
