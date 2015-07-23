#include "spfs_fuse_track.h"
#include "spfs_spotify.h"

static int is_starred_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	char *str = spotify_track_is_starred(SPFS_SP_SESSION, e->parent->auxdata) ? "1" : "0";
	READ_OFFSET(str, buf, size, offset);
	return size;
}

static int is_local_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	char *str = spotify_track_is_local(SPFS_SP_SESSION, e->parent->auxdata) ? "1" : "0";
	READ_OFFSET(str, buf, size, offset);
	return size;
}

static int is_autolinked_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	char *str = spotify_track_is_autolinked(SPFS_SP_SESSION, e->parent->auxdata) ? "1" : "0";
	READ_OFFSET(str, buf, size, offset);
	return size;
}

static int disc_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	gchar *str = g_strdup_printf("%d",
			spotify_track_disc(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int index_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	gchar *str = g_strdup_printf("%d",
			spotify_track_index(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int popularity_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	gchar *str = g_strdup_printf("%d",
			spotify_track_popularity(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int duration_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	gchar *str = g_strdup_printf("%d",
			spotify_track_duration(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int pcm_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
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

spfs_entity *create_track_browse_dir(sp_track *track) {
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

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("popularity", popularity_read));

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("index", index_read));

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("disc", disc_read));

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("starred", is_starred_read));

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("local", is_local_read));

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("autolinked", is_autolinked_read));

	spfs_entity_dir_add_child(track_browse_dir, track_dir);
	return track_dir;
}


