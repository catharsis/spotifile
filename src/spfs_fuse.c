#include "spotify-fs.h"
#include "spfs_spotify.h"
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <libgen.h>
#include <glib.h>
extern time_t g_logged_in_at; /*FIXME: get rid of this */

struct spfs_data {
	sp_session *session;
	spfs_entity *root;
};

#define SPFS_DATA ((struct spfs_data *)(fuse_get_context()->private_data))
#define SPFS_SP_SESSION ((sp_session *)((SPFS_DATA)->session))


size_t connection_file_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int connection_file_getattr(const char *path, struct stat *statbuf);


int search_file_getattr(const char *path, struct stat *statbuf);
int browse_file_getattr(const char *path, struct stat *statbuf);

int spfs_dir_getattr(const char *path, struct stat *statbuf)
{
	memset(statbuf, 0, sizeof(struct stat));
	statbuf->st_mode = S_IFDIR | 0755;
	statbuf->st_nlink = 2;
	return 0;
}

int spfs_getattr(const char *path, struct stat *statbuf)
{
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	if (e != NULL && e->getattr != NULL) {
		return (e->getattr)(path, statbuf);
	}
	return spfs_dir_getattr(path, statbuf);
}

int connection_file_getattr(const char* path, struct stat *statbuf)
{
	if (strcmp("/connection", path) != 0) {
		g_warn_if_reached();
		return -ENOENT;
	}
	else {
		statbuf->st_mode = S_IFREG | 0444;
		statbuf->st_nlink = 1;
		statbuf->st_size = 64;
		statbuf->st_mtime = g_logged_in_at;
		return 0;
	}
}

int browse_dir_getattr(const char *path, struct stat *statbuf)
{
	if (g_strcmp0("/browse", path) == 0) {
		statbuf->st_mtime = time(NULL);
		statbuf->st_mode = S_IFDIR | 0755;
	}
	else {
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_mtime = time(NULL);
	}
	return 0;
}

int search_dir_getattr(const char *path, struct stat *statbuf)
{
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
	else {
		g_return_val_if_reached(-ENOENT);
	}
	return 0;
}

size_t connection_file_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	char *state_str =
		g_strdup(spotify_connectionstate_str(spotify_connectionstate(session)));

	size_t len = 0;

	if ((len = strlen(state_str)) > (size_t)offset) {
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
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	g_return_val_if_fail(e->type == SPFS_FILE, -ENOENT);

	if (e->e.file->read == NULL) {
		g_return_val_if_reached(-ENOENT);
	}

	return e->e.file->read(path, buf, size, offset, fi);

}

int spfs_readlink(const char *path, char *buf, size_t len) {
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	g_return_val_if_fail(e != NULL, -ENOENT);
	g_return_val_if_fail(e->type == SPFS_LINK, -ENOENT);


	spfs_entity *te = e->e.link->target;
	gchar *target_path = spfs_entity_get_full_path(te);
	strncpy(buf, target_path, len);
	g_free(target_path);
	return 0;
}

int browse_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {

	gchar *dir= g_path_get_dirname(path);
	if (g_strcmp0(dir, "/browse/artists") == 0) {
		/*FIXME: hardcoded artists :(*/
		gchar *linkstr = g_path_get_basename(path);
		sp_link *link = spotify_link_create_from_string(linkstr);
		g_free(linkstr);
		if (!link) {
			g_warning("no link");
			return -ENOENT;
		}
		sp_artist *artist = spotify_link_as_artist(link);
		if (!artist) {
			g_warning("no artist");
			return -ENOENT;
		}

		filler(buf, "name", NULL, 0);
		filler(buf, "biography", NULL, 0);
	}
	g_free(dir);
	return 0;
}
int search_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);

	gchar *dir = g_path_get_dirname(path);
	if (strcmp(dir, "/search/artists") == 0) {
		/*artist query*/
		gchar *query = g_path_get_basename(path);
		GSList *artists;
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		g_debug("querying for artist: %s", query);
		artists = spotify_artist_search(session, query);
		g_free(query);
		if (artists != NULL) {
			GSList *tmp = artists;
			//spfs_entity *artist_browse_dir = spfs_entity_find_path(SPFS_DATA->root, "/browse/artists");
			while (tmp != NULL) {
				sp_artist *artist = (sp_artist *) tmp->data;
				gchar *p;
				gchar *artist_name = g_strdup(spotify_artist_name(artist));
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
			return -ENOENT;
		}
	}
	g_free(dir);
	return 0;
}

int spfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	g_debug("readdir path:%s ", path);

	spfs_entity *e = NULL;
	gchar *searchpath = g_strdup(path), *tmp;

	/*
	 * Go up the tree until we find something that can handle this request.
	 * Since not all entities are materialized (but rather, generated on-the-fly), it is
	 * not necessarily an error that we don't find an exact match here. Of
	 * course, this means that the delegate needs to take care that the path received
	 * actually something sensible from its own POV.
	 */
	while ( (e = spfs_entity_find_path((SPFS_DATA)->root, searchpath)) == NULL) {
		tmp = searchpath;
		searchpath = g_path_get_dirname(searchpath);
		g_free(tmp);
	}

	g_free(searchpath);
	if (!e || e->type != SPFS_DIR) {
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	/* Autofill children*/
	GHashTableIter iter;
	gpointer epath, sube;
	g_hash_table_iter_init (&iter, e->e.dir->children);
	while (g_hash_table_iter_next (&iter, &epath, &sube))
	{
		filler(buf, epath, NULL, 0);
	}
	if (e->e.dir->readdir != NULL) {
		return e->e.dir->readdir(path, buf, filler, offset, fi);
	}
	return 0;
}

void *spfs_init(struct fuse_conn_info *conn)
{
	sp_session *session = NULL;
	struct spfs_data *data = g_new0(struct spfs_data, 1);
	struct fuse_context *context = fuse_get_context();
	struct spotifile_config *conf= (struct spotifile_config *) context->private_data;

	spfs_entity *root = spfs_entity_root_create(spfs_dir_getattr, NULL);

	spfs_entity_dir_add_child(root,
			spfs_entity_file_create(
				"connection",
				connection_file_getattr,
				connection_file_read
			));

	spfs_entity *searchdir = spfs_entity_dir_create(
				"search",
				search_dir_getattr,
				search_dir_readdir
				);

	spfs_entity_dir_add_child(searchdir,
			spfs_entity_dir_create(
				"artists",
				search_dir_getattr,
				search_dir_readdir
				));

	spfs_entity_dir_add_child(root, searchdir);

	spfs_entity *browsedir = spfs_entity_dir_create(
				"browse",
				browse_dir_getattr,
				browse_dir_readdir
				);

	spfs_entity_dir_add_child(browsedir,
			spfs_entity_dir_create(
				"artists",
				browse_dir_getattr,
				browse_dir_readdir
				));

	spfs_entity_dir_add_child(root, browsedir);

	data->root = root;
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
	spfs_entity_destroy(spfsdata->root);
	g_info("%s destroyed", application_name);
}


struct fuse_operations spfs_operations = {
	.init = spfs_init,
	.destroy = spfs_destroy,
	.getattr = spfs_getattr,
	.read = spfs_read,
	.readdir = spfs_readdir,
	.readlink = spfs_readlink
};

struct fuse_operations spfs_get_fuse_operations() {
	return spfs_operations;
}
