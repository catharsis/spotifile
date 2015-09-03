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
#include <lame/lame.h>
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

int spfs_access(const char *path, int mask)
{
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	if (!e)
		return -ENOENT;

	if (mask == F_OK)
		return 0;

	if (mask & W_OK)
		return -EACCES;

	if (mask & X_OK && e->type == SPFS_FILE)
		return -EACCES;

	if (mask & R_OK || mask & X_OK)
		return 0;

	return -EINVAL;
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
	fi->fh = SPFS_ENT2FH(e);
	int ret = 0;
	if (e->e.file->ops->open != NULL) {
		ret = e->e.file->ops->open(path, fi);
	}

	if (G_LIKELY(!ret)) {
		e->refs++;
		fi->direct_io = spfs_entity_get_direct_io(e);
	}
	else {
		fi->fh = 0;
	}
	return ret;

}

int spfs_release(const char *path, struct fuse_file_info *fi)
{
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	if (e->e.file->ops->release != NULL)
		/* "The return value of release is ignored." */
		e->e.file->ops->release(path, fi);
	e->refs--;
	return 0;
}

int spfs_releasedir(const char *path, struct fuse_file_info *fi)
{
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	if (e->e.dir->ops->release != NULL)
		/* "The return value of release is ignored." */
		e->e.dir->ops->release(path, fi);
	e->refs--;
	return 0;
}


int spfs_opendir(const char *path, struct fuse_file_info *fi)
{
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	g_return_val_if_fail(e != NULL, -ENOENT);
	g_return_val_if_fail(e->type == SPFS_DIR, -EINVAL);
	fi->fh = SPFS_ENT2FH(e);
	return 0;
}

int spfs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	g_return_val_if_fail(e->type == SPFS_FILE, -EISDIR);

	if (e->e.file->ops->read == NULL) {
		g_return_val_if_reached(-EINVAL);
	}

	return e->e.file->ops->read(path, buf, size, offset, fi);
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
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	if (!e || e->type != SPFS_DIR) {
		return -ENOENT;
	}
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	spfs_dir *dir = e->e.dir;
	g_debug("filling existing dir %s ", path);
	if (dir->ops->readdir != NULL) {
		ret = dir->ops->readdir(path, buf, filler, offset, fi);
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
	data->playlist_track_format = g_strdup(conf->playlist_track_format);

	spfs_entity *root = spfs_entity_root_create(NULL);

	spfs_entity_dir_add_child(root,
			spfs_entity_file_create(
				"connection",
				&(struct spfs_file_ops){.read = connection_read}
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

	spfs_entity * playlists_dir = spfs_entity_dir_create("playlists", NULL);
	spfs_entity_dir_add_child(root, playlists_dir);

	spfs_entity_dir_add_child(playlists_dir,
			spfs_entity_dir_create(
				"meta",
				&(struct spfs_dir_ops){.readdir = playlists_meta_dir_readdir}
				)
			);

	spfs_entity_dir_add_child(playlists_dir,
			spfs_entity_dir_create(
				"music",
				&(struct spfs_dir_ops){.readdir = playlists_music_dir_readdir}
				)
			);

	data->root = root;
	g_message("%s initialising ...", application_name);
#ifdef HAVE_LIBLAME
	g_message("Using LAME %s...", get_lame_version());
#endif
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

#define W_UNIMPLEMENTED() g_warning("Unimplemented function %s called!", __func__);

int spfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_mkdir(const char *path, mode_t mode)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_unlink(const char *path)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_rmdir(const char *path)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_symlink(const char *path, const char *target)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_rename(const char *path, const char *newpath)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_hardlink(const char *path, const char *target)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_chmod(const char *path, mode_t mode)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_chown(const char *path, uid_t uid, gid_t gid)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_truncate(const char *path, off_t size)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}


int spfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_statfs(const char *path, struct statvfs *stbuf)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_flush(const char *path, struct fuse_file_info *fi)
{
	W_UNIMPLEMENTED();
	return 0;
}

int spfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_fsyncdir(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_utimens(const char *path, const struct timespec tv[2])
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_bmap(const char *path, size_t blocksize, uint64_t *idx)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

int spfs_poll(const char *path, struct fuse_file_info *fi, struct fuse_pollhandle *ph, unsigned *reventsp)
{
	W_UNIMPLEMENTED();
	return -EACCES;
}

struct fuse_operations spfs_operations = {

	/* Don't bother calculating the path
	 * on our account, we use handles.
	 *
	 * Sorry about the #if'ing mess.
	 */
#if FUSE_MAJOR_VERSION > 2 || ( FUSE_MAJOR_VERSION == 2 && FUSE_MINOR_VERSION >= 8 )
	.flag_nullpath_ok = 1,
#endif  // >= FUSE 2.8
#if FUSE_MAJOR_VERSION > 2 || ( FUSE_MAJOR_VERSION == 2 && FUSE_MINOR_VERSION >= 9 )
	.flag_nopath = 1,
#endif  // >= FUSE 2.9

	.getattr = spfs_getattr,
	.readlink = spfs_readlink,
	.mknod = spfs_mknod,
	.mkdir = spfs_mkdir,
	.unlink = spfs_unlink,
	.rmdir  = spfs_rmdir,
	.symlink = spfs_symlink,
	.rename = spfs_rename,
	.link = spfs_hardlink,
	.chmod = spfs_chmod,
	.chown = spfs_chown,
	.truncate = spfs_truncate,
	.open = spfs_open,
	.read = spfs_read,
	.write = spfs_write,
	.statfs = spfs_statfs,
	.flush = spfs_flush,
	.release = spfs_release,
	.fsync = spfs_fsync,
	//{set, get, list, remove}xattr
	.opendir = spfs_opendir,
	.readdir = spfs_readdir,
	.releasedir = spfs_releasedir,
	.fsyncdir = spfs_fsyncdir,
	.init = spfs_init,
	.destroy = spfs_destroy,
	.access = spfs_access,
	.create = spfs_create,
	.ftruncate = spfs_ftruncate,
	.fgetattr = spfs_fgetattr,
	.utimens = spfs_utimens,
	.bmap = spfs_bmap,
	.ioctl = spfs_ioctl,
	.poll = spfs_poll,
};

struct fuse_operations spfs_get_fuse_operations() {
	return spfs_operations;
}
