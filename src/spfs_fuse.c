#include "spotify-fs.h"
#include "spfs_spotify.h"
#include "spfs_fuse.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <libgen.h>
#include <glib.h>
extern time_t g_logged_in_at;

struct spfs_data {
	sp_session *session;
	GSList *search_result;
};

#define SPFS_DATA ((struct spfs_data *)(fuse_get_context()->private_data))
#define SPFS_SP_SESSION ((sp_session *)((SPFS_DATA)->session))
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
	sp_session *session = SPFS_SP_SESSION;
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

int spfs_open(const char *path, struct fuse_file_info *finfo) {
	g_debug("open for %s", path);

}
int spfs_readlink(const char *path, char *buf, size_t len) {
	g_debug("readlink for %s", path);
	GSList *search_result = SPFS_DATA->search_result, *tmp = search_result;
	gchar *basename = g_path_get_basename(path);
	sp_artist *artist = NULL; 
	while (tmp != NULL) {
		const gchar *name = spotify_artist_name(tmp->data);
		if (g_strcmp0(name, basename) == 0) {
			artist = tmp->data;
			break;
		}
		tmp = g_slist_next(tmp);
	}
	g_return_val_if_fail(artist != NULL, 1);

	sp_link *link = spotify_link_create_from_artist(artist);
	gchar *target_dir = "../../../browse/";
	size_t tlen = strlen(target_dir);
	strncpy(buf, target_dir, tlen);

	spotify_link_as_string(link, buf + tlen, len - tlen);
	g_debug("target:  %s", buf);
	g_free(basename);
	return 0;
}

int spfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	/*TODO: remove the hardcoding of paths, parse
	 * spfs_files instead*/
	sp_session *session = SPFS_SP_SESSION;
	g_debug("readdir path:%s ", path);
	g_return_val_if_fail(session != NULL, 0);
	char *dirname_path_copy = strdup(path);
	char *basename_path_copy = strdup(path);
	int ret = 0;
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
		char *query= NULL;
		GSList *artists;
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		query = g_strdup(basename(basename_path_copy));
		g_debug("querying for artist: %s", query);
		artists = spotify_artist_search(session, query);
		SPFS_DATA->search_result = artists;
		g_free(query);
		if (artists != NULL) {
			GSList *tmp = artists;
			while (tmp != NULL) {
				gchar *p;
				gchar *artist_name = g_strdup(spotify_artist_name(tmp->data));
				/* Replace slashes, since they are reserved and unescapable in
				 * directory names.
				 * Possibly, we could replace this with some
				 * unicode symbol that resembles the usual slash. But that's for
				 * another rainy day*/
				p = artist_name;
				while ((p = strchr(p, '/')) != NULL) {
					*p = ' ';
				}
				filler(buf, artist_name, NULL, 0);
				g_free(artist_name);
				tmp = g_slist_next(tmp);
			}
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


void *spfs_init(struct fuse_conn_info *conn)
{
	sp_session *session = NULL;
	struct spfs_data *data = g_new0(struct spfs_data, 1);
	struct fuse_context *context = fuse_get_context();
	struct spotifile_config *conf= (struct spotifile_config *) context->private_data;
	g_info("%s initialising ...", application_name);
	session = spotify_session_init(conf->spotify_username, conf->spotify_password, NULL);
	data->session = session;
	spotify_threads_init(session);
	g_info("%s initialised", application_name);
	return data;

}

void spfs_destroy(void *userdata)
{
	struct spfs_data * spfsdata = (struct spfs_data *)userdata;
	sp_session *session = spfsdata->session;
	spotify_session_destroy(session);
	spotify_threads_destroy();
	g_info("%s destroyed", application_name);
}


struct fuse_operations spfs_operations = {
	.init = spfs_init,
	.destroy = spfs_destroy,
	.getattr = spfs_getattr,
	.open = spfs_open,
	.read = spfs_read,
	.readdir = spfs_readdir,
	.readlink = spfs_readlink
};

struct fuse_operations spfs_get_fuse_operations() {
	return spfs_operations;
}
