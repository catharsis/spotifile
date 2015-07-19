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

int browse_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);


static spfs_entity *create_artist_browse_dir(sp_artist *artist) {
	g_return_val_if_fail(artist != NULL, NULL);
	spfs_entity *artist_browse_dir = spfs_entity_find_path(SPFS_DATA->root, "/browse/artists");
	sp_link *link = spotify_link_create_from_artist(artist);
	if (!link) {
		g_return_val_if_reached(NULL);
	}

	gchar artist_linkstring[1024] = {0, };
	spotify_link_as_string(link, artist_linkstring, 1024);

	if (spfs_entity_dir_has_child(artist_browse_dir->e.dir, artist_linkstring)) {
		return NULL;
	}

	spfs_entity *artist_dir = spfs_entity_dir_create(artist_linkstring, NULL);

	spfs_entity_dir_add_child(artist_dir, spfs_entity_file_create("name", NULL));
	spfs_entity_dir_add_child(artist_dir, spfs_entity_file_create("biography", NULL));
	spfs_entity_dir_add_child(artist_browse_dir, artist_dir);
	return artist_dir;
}

static void fill_dir_children(spfs_dir *dir, void *buf, fuse_fill_dir_t filler) {
	g_return_if_fail(dir != NULL);

	if (g_hash_table_size(dir->children) == 0) {
		g_debug("no children for directory");
		return;
	}

	/* Autofill children*/
	GHashTableIter iter;
	gpointer epath, sube;
	g_hash_table_iter_init (&iter, dir->children);
	while (g_hash_table_iter_next (&iter, &epath, &sube))
	{
		spfs_entity *child = (spfs_entity *) sube;
		struct stat statbuf;
		memset(&statbuf, 0, sizeof(struct stat));
		spfs_entity_stat(child, &statbuf);
		filler(buf, epath, &statbuf, 0);
	}
}

int spfs_getattr(const char *path, struct stat *statbuf)
{
	g_debug ("getattr: %s", path);
	memset(statbuf, 0, sizeof(struct stat));
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	if (e != NULL) {
		spfs_entity_stat(e, statbuf);
		return 0;
	}
	return -ENOENT;
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

int spfs_open(const char *path, struct fuse_file_info *fi)
{
	g_debug("open: %s", path);
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	g_return_val_if_fail(e != NULL, -ENOENT);
	fi->fh = (uint64_t)e;
	return 0;

}

int spfs_opendir(const char *path, struct fuse_file_info *fi)
{
	g_debug("opendir: %s", path);
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	g_return_val_if_fail(e != NULL, -ENOENT);
	g_return_val_if_fail(e->type == SPFS_DIR, -EINVAL);
	fi->fh = (uint64_t)e;
	return 0;
}

int spfs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	spfs_entity *e = (spfs_entity *) fi->fh;
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

	strncpy(buf, e->e.link->target, len);
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
	}
	g_free(dir);
	return 0;
}

/*readdir for a single playlist directory*/
int playlist_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {
	return 0;
}

/*readdir for the "root" playlists directory*/
int playlists_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	spfs_entity *playlists_dir = (spfs_entity *)fi->fh;

	GSList *playlists = spotify_get_playlists(session);
	if (playlists != NULL) {
		GSList *tmp = playlists;
		while (tmp != NULL) {
			sp_playlist * pl = tmp->data;
			const gchar *name = spotify_playlist_name(pl);
			if (!spfs_entity_dir_has_child(playlists_dir->e.dir, name)) {
				spfs_entity * pld =
					spfs_entity_dir_create(name, playlist_dir_readdir);

				spfs_entity_dir_add_child(playlists_dir, pld);
			}
			tmp = g_slist_next(tmp);
		}
		g_slist_free(playlists);
	}
	return 0;
}

int spfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	int ret = 0;
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	g_debug("readdir path:%s ", path);
	spfs_entity *e = (spfs_entity *)fi->fh;
	if (!e || e->type != SPFS_DIR) {
		return -ENOENT;
	}
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	spfs_dir *dir = e->e.dir;
	g_debug("filling existing dir %s ", path);
	if (dir->readdir != NULL) {
		ret = dir->readdir(path, buf, filler, offset, fi);
	}
	fill_dir_children(e->e.dir, buf, filler);
	return ret;
}

void *spfs_init(struct fuse_conn_info *conn)
{
	sp_session *session = NULL;
	struct spfs_data *data = g_new0(struct spfs_data, 1);
	struct fuse_context *context = fuse_get_context();
	struct spotifile_config *conf= (struct spotifile_config *) context->private_data;

	spfs_entity *root = spfs_entity_root_create(NULL);

	spfs_entity_dir_add_child(root,
			spfs_entity_file_create(
				"connection",
				connection_file_read
			));

	spfs_entity *browsedir = spfs_entity_dir_create(
				"browse",
				NULL
				);

	spfs_entity_dir_add_child(browsedir,
			spfs_entity_dir_create(
				"artists",
				NULL
				));

	spfs_entity_dir_add_child(root, browsedir);

	spfs_entity_dir_add_child(root,
			spfs_entity_dir_create(
				"playlists",
				playlists_dir_readdir
				)
			);

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
	.readlink = spfs_readlink,
	.open = spfs_open,
	.opendir = spfs_opendir
};

struct fuse_operations spfs_get_fuse_operations() {
	return spfs_operations;
}
