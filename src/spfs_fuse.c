#include "spotify-fs.h"
#include "spfs_spotify.h"
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
#include "spfs_path.h"
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


int connection_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int pcm_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int duration_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

static gchar *relpath(spfs_entity *from, spfs_entity *to) {
	gchar *frompath = spfs_entity_get_full_path(from);
	gchar *topath = spfs_entity_get_full_path(to);

	gchar * relpath = spfs_path_get_relative_path(frompath, topath);
	g_free(frompath);
	g_free(topath);
	return relpath;
}

static spfs_entity *create_track_browse_dir(sp_track *track) {
	g_return_val_if_fail(track != NULL, NULL);
	spfs_entity *track_browse_dir = spfs_entity_find_path(SPFS_DATA->root, "/browse/tracks");
	sp_link *link = spotify_link_create_from_track(track);

	g_return_val_if_fail(link != NULL, NULL);
	gchar track_linkstring[1024] = {0, };
	spotify_link_as_string(link, track_linkstring, 1024);
	if (spfs_entity_dir_has_child(track_browse_dir->e.dir, track_linkstring)) {
		return NULL;
	}
	spfs_entity *track_dir = spfs_entity_dir_create(track_linkstring, NULL);
	track_dir->auxdata = track;
	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("pcm", pcm_read));

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("duration", duration_read));

	spfs_entity_dir_add_child(track_browse_dir, track_dir);
	return track_dir;
}

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

#define READ_OFFSET(_Str, _Buf, _Sz, _Off) do {\
	if ((strlen(_Str)) > (size_t)_Off) { \
		if ( _Off + _Sz > strlen(_Str)) \
			_Sz = strlen(_Str) - _Off; \
		memcpy(_Buf, _Str + _Off, _Sz); \
	} \
	else _Sz = 0;  } while (0)


int duration_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	gchar *str = g_strdup_printf("%d",
			spotify_track_duration(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

int pcm_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	sp_session *session = SPFS_SP_SESSION;

	if (!spotify_play_track(session, e->parent->auxdata)) {
		g_warning("Failed to play track!");
		return 0;
	}
	memset(buf, 0, size);
	size_t read = spotify_get_audio(buf, size);
	return read;
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
	g_debug("open: %s", path);
	spfs_entity *e = spfs_entity_find_path((SPFS_DATA)->root, path);
	g_return_val_if_fail(e != NULL, -ENOENT);
	fi->direct_io = spfs_entity_get_direct_io(e);
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
	g_debug("read: %s", path);
	spfs_entity *e = (spfs_entity *) fi->fh;
	g_return_val_if_fail(e->type == SPFS_FILE, -EISDIR);

	if (e->e.file->read == NULL) {
		g_return_val_if_reached(-EINVAL);
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

/*readdir for a single playlist directory*/
int playlist_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {

	spfs_entity *e = (spfs_entity *)fi->fh;
	sp_playlist *pl = (sp_playlist *)e->auxdata;

	GSList *tracks = spotify_playlist_get_tracks(pl);
	if (tracks != NULL) {
		GSList *tmp = tracks;
		while (tmp != NULL) {
			sp_track *track = (sp_track *)tmp->data;
			const gchar *trackname = spotify_track_name(track);
			spfs_entity *track_browse_dir = create_track_browse_dir(track);
			if (!spfs_entity_dir_has_child(e->e.dir, trackname)) {
				spfs_entity *tracklink = spfs_entity_link_create(trackname, NULL);
				spfs_entity_dir_add_child(e, tracklink);
				spfs_entity_link_set_target(tracklink,
						relpath(e, track_browse_dir));
			}
			tmp = g_slist_next(tmp);
		}
		g_slist_free(tracks);
	}
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

				pld->auxdata = pl;

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
