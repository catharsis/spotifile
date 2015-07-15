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


static void fill_dir_children(spfs_dir *dir, void **buf, fuse_fill_dir_t filler) {
	g_return_if_fail(dir != NULL);
	/* Autofill children*/
	GHashTableIter iter;
	gpointer epath, sube;
	g_hash_table_iter_init (&iter, dir->children);
	while (g_hash_table_iter_next (&iter, &epath, &sube))
	{
		filler(*buf, epath, NULL, 0);
	}
}
static spfs_entity *find_existing_parent(spfs_entity *root, const char *path) {
	/* FIXME: This comment doesn't belong here
	 * Go up the tree until we find something that can handle this request.
	 * Since not all entities are materialized (but rather, generated on-the-fly), it is
	 * not necessarily an error that we don't find an exact match here. Of
	 * course, this means that the delegate needs to take care that the path received
	 * actually something sensible from its own POV.
	 */
	spfs_entity *e = NULL;
	gchar *searchpath = g_strdup(path), *tmp;
	while ( (e = spfs_entity_find_path((SPFS_DATA)->root, searchpath)) == NULL) {
		tmp = searchpath;
		searchpath = g_path_get_dirname(searchpath);
		g_free(tmp);
	}

	g_free(searchpath);
	return e;
}


int spfs_dir_getattr(const char *path, struct stat *statbuf)
{
	memset(statbuf, 0, sizeof(struct stat));
	statbuf->st_mode = S_IFDIR | 0755;
	statbuf->st_nlink = 2;
	return 0;
}

int spfs_getattr(const char *path, struct stat *statbuf)
{
	g_debug ("getattr: %s", path);
	spfs_entity *e = find_existing_parent((SPFS_DATA)->root, path);
	if (e != NULL) {
		if (e->getattr != NULL)
			return (e->getattr)(path, statbuf);
		else if (e->type == SPFS_DIR)
			return spfs_dir_getattr(path, statbuf);
	}

	return -ENOENT;
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
	g_debug("readlink: %s", path);
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

	g_debug ("browse readdir: %s", path);
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
	g_debug ("search readdir: %s", path);
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);

	gchar *dir = g_path_get_dirname(path);
	if (g_strcmp0(dir, "/search/artists") == 0) {
		/*artist query*/
		gchar *query = g_path_get_basename(path);
		GSList *artists;
		g_debug("querying for artist: %s", query);
		artists = spotify_artist_search(session, query);
		if (artists != NULL) {
			GSList *tmp = artists;
			spfs_entity *artist_browse_dir = spfs_entity_find_path(SPFS_DATA->root, "/browse/artists");
			spfs_entity *artist_search_dir = spfs_entity_find_path(SPFS_DATA->root, "/search/artists");
			spfs_entity *this_search = spfs_entity_dir_create(query, NULL, NULL);
			spfs_entity_dir_add_child(artist_search_dir,
					this_search);
			while (tmp != NULL) {
				sp_artist *artist = (sp_artist *) tmp->data;
				gchar *artist_name = g_strdup(spotify_artist_name(artist));
				/* Replace slashes, since they are reserved and unescapable in
				 * directory names.
				 * Possibly, we could replace this with some
				 * unicode symbol that resembles the usual slash. But that's for
				 * another rainy day
				gchar *p = artist_name;
				while ((p = strchr(p, '/')) != NULL) {
					*p = ' ';
				}
				*/
				/* Create the target artist directory*/
				sp_link *link = spotify_link_create_from_artist(artist);
				if (!link) {
					g_warning("no link");
					return -ENOENT;
				}

				gchar artist_linkstring[1024] = {0, };
				spotify_link_as_string(link, artist_linkstring, 1024);
				spfs_entity *artist_dir = spfs_entity_dir_create(artist_linkstring,
							browse_dir_getattr,
							browse_dir_readdir);
				spfs_entity_dir_add_child(artist_browse_dir, artist_dir);

				spfs_entity *artist_search_link = spfs_entity_link_create(artist_name,
						search_dir_getattr,
						spfs_readlink);
				spfs_entity_link_set_target(artist_search_link, artist_dir);
				spfs_entity_dir_add_child(this_search, artist_search_link);

				g_free(artist_name);
				tmp = g_slist_next(tmp);
			}
			fill_dir_children(this_search->e.dir, &buf, filler);
			g_free(query);
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

	spfs_entity *e = find_existing_parent((SPFS_DATA)->root, path);
	if (!e || e->type != SPFS_DIR) {
		return -ENOENT;
	}

	spfs_dir *dir = e->e.dir;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if (g_hash_table_size(dir->children) > 0) {
		fill_dir_children(dir, &buf, filler);
		return 0;
	}
	else if (dir->readdir != NULL) {
		return dir->readdir(path, buf, filler, offset, fi);
	}
	else {
		return -ENOENT;
	}
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
