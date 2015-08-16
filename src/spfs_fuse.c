#include "spotify-fs.h"
#include "spfs_fuse.h"
#include "spfs_fuse_playlist.h"
#include "spfs_path.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <libgen.h>
#include <glib.h>
extern time_t g_logged_in_at; /*FIXME: get rid of this */

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
	memset(statbuf, 0, sizeof(struct stat));
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	if (e != NULL) {
		spfs_entity_stat(e, statbuf);
		return 0;
	}
	return -ENOENT;
}

int connection_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	char *state_str =
		g_strdup(spotify_connectionstate_str(spotify_connectionstate(session)));

	READ_OFFSET(state_str, buf, size, offset);
	g_free(state_str);
	return size;
}

int spfs_open(const char *path, struct fuse_file_info *fi)
{
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	g_return_val_if_fail(e != NULL, -ENOENT);
	fi->direct_io = spfs_entity_get_direct_io(e);
	fi->fh = (uint64_t)e;
	return 0;

}

int spfs_opendir(const char *path, struct fuse_file_info *fi)
{
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
	g_return_val_if_fail(e->type == SPFS_FILE, -EISDIR);

	if (e->e.file->read == NULL) {
		g_return_val_if_reached(-EINVAL);
	}

	return e->e.file->read(path, buf, size, offset, fi);
}

int spfs_readlink(const char *path, char *buf, size_t len) {
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	g_return_val_if_fail(e != NULL, -ENOENT);
	g_return_val_if_fail(e->type == SPFS_LINK, -ENOENT);

	strncpy(buf, e->e.link->target, len);
	return 0;
}


int spfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	int ret = 0;
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
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
				connection_read
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

	spfs_entity_dir_add_child(browsedir,
			spfs_entity_dir_create(
				"tracks",
				NULL
				));

	spfs_entity_dir_add_child(browsedir,
			spfs_entity_dir_create(
				"albums",
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
	g_message("%s initialising ...", application_name);
	session = spotify_session_init(conf->spotify_username, conf->spotify_password, NULL);
	data->session = session;
	spotify_threads_init(session);
	g_message("%s initialised", application_name);
	return data;

}

void spfs_destroy(void *userdata)
{
	struct spfs_data * spfsdata = (struct spfs_data *)userdata;
	sp_session *session = spfsdata->session;
	spotify_threads_destroy();
	spotify_session_destroy(session);
	spfs_entity_destroy(spfsdata->root);
	g_message("%s destroyed", application_name);
}


struct fuse_operations spfs_operations = {
	.init = spfs_init,
	.destroy = spfs_destroy,
	.getattr = spfs_getattr,
	.read = spfs_read,
	.readdir = spfs_readdir,
	.readlink = spfs_readlink,
	.open = spfs_open,
	.opendir = spfs_opendir,

	/* Don't bother calculating the path
	 * on our account, we use handles */
	.flag_nullpath_ok = 1,
	.flag_nopath = 1,
};

struct fuse_operations spfs_get_fuse_operations() {
	return spfs_operations;
}
