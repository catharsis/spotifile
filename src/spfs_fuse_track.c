#include "spfs_fuse_track.h"
#include "spfs_fuse_artist.h"
#include "spfs_fuse_audiofile.h"
#include "spfs_spotify.h"

static int is_starred_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	char *str = spotify_track_is_starred(SPFS_SP_SESSION, e->parent->auxdata) ? "1\n" : "0\n";
	READ_OFFSET(str, buf, size, offset);
	return size;
}

static int is_local_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	char *str = spotify_track_is_local(SPFS_SP_SESSION, e->parent->auxdata) ? "1\n" : "0\n";
	READ_OFFSET(str, buf, size, offset);
	return size;
}

static int is_autolinked_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	char *str = spotify_track_is_autolinked(SPFS_SP_SESSION, e->parent->auxdata) ? "1\n" : "0\n";
	READ_OFFSET(str, buf, size, offset);
	return size;
}

static int offlinestatus_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	char *str = g_strdup_printf("%s\n", spotify_track_offline_status_str(
			spotify_track_offline_get_status(e->parent->auxdata)
			));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int name_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	gchar *track_name = spotify_track_name(e->parent->auxdata);
	gchar *str = g_strdup_printf("%s\n", track_name);
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	g_free(track_name);
	return size;
}

static int disc_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	gchar *str = g_strdup_printf("%d\n",
			spotify_track_disc(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int index_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	gchar *str = g_strdup_printf("%d\n",
			spotify_track_index(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int popularity_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	gchar *str = g_strdup_printf("%d\n",
			spotify_track_popularity(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int duration_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	gchar *str = g_strdup_printf("%d\n",
			spotify_track_duration(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

int artists_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	sp_track *track = e->parent->auxdata;
	int num_artists = spotify_track_num_artists(track);
	for (int i = 0; i < num_artists; ++i) {
		sp_artist *artist = spotify_track_artist(track, i);
		gchar *artistname = spotify_artist_name(artist);
		spfs_entity *artist_browse_dir = create_artist_browse_dir(artist);
		/*
		 * FIXME: Deal with duplicates (artists with the same name).
		 */
		if (!spfs_entity_dir_has_child(e->e.dir, artistname)) {
			spfs_entity *artist_link = spfs_entity_link_create(artistname, NULL);
			spfs_entity_dir_add_child(e, artist_link);
			gchar *rpath = relpath(e, artist_browse_dir);
			spfs_entity_link_set_target(artist_link, rpath);
			g_free(rpath);
		}

		g_free(artistname);
	}
	return 0;
}

#ifdef HAVE_LIBLAME
spfs_entity *create_track_mp3_file(const gchar *name, sp_track *track) {
	g_return_val_if_fail(name != NULL, NULL);
	g_return_val_if_fail(track != NULL, NULL);
	spfs_entity * mp3 = spfs_entity_file_create(name,
			&(struct spfs_file_ops){
			.open = audiofile_open,
			.read = mp3_read,
			.release = audiofile_release
			});

	mp3->auxdata = track;

	return mp3;

}
#endif

spfs_entity *create_track_wav_file(const gchar *name, sp_track *track) {
	g_return_val_if_fail(name != NULL, NULL);
	g_return_val_if_fail(track != NULL, NULL);
	spfs_entity * wav = spfs_entity_file_create(name,
			&(struct spfs_file_ops){
			.open = audiofile_open,
			.read = wav_read,
			.release = audiofile_release
			});

	wav->auxdata = track;

	return wav;
}

spfs_entity *create_track_browse_dir(sp_track *track) {
	g_return_val_if_fail(track != NULL, NULL);
	spfs_entity *track_browse_dir = spfs_entity_find_path(SPFS_DATA->root, "/browse/tracks");
	sp_link *link = spotify_link_create_from_track(track);

	g_return_val_if_fail(link != NULL, NULL);
	gchar track_linkstring[1024] = {0, };
	spotify_link_as_string(link, track_linkstring, 1024);
	if (spfs_entity_dir_has_child(track_browse_dir->e.dir, track_linkstring)) {
		return spfs_entity_dir_get_child(track_browse_dir->e.dir, track_linkstring);
	}
	spfs_entity *track_dir = spfs_entity_dir_create(track_linkstring, NULL);
	track_dir->auxdata = track;

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("name",
				&(struct spfs_file_ops) {.read = name_read}
				)
			);

	spfs_entity_dir_add_child(track_dir,
			create_track_wav_file("track.wav", track));

	spfs_entity_dir_add_child(track_dir,
			create_track_mp3_file("track.mp3", track));

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("duration",
				&(struct spfs_file_ops) {.read = duration_read})
			);

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("popularity",
				&(struct spfs_file_ops) {.read = popularity_read})
			);

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("index",
				&(struct spfs_file_ops) {.read = index_read})
			);

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("disc",
				&(struct spfs_file_ops) {.read = disc_read})
			);

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("starred",
				&(struct spfs_file_ops) {.read = is_starred_read})
			);

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("local",
				&(struct spfs_file_ops) {.read = is_local_read})
			);

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("autolinked",
				&(struct spfs_file_ops) {.read = is_autolinked_read})
			);

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_file_create("offlinestatus",
				&(struct spfs_file_ops) {.read = offlinestatus_read})
			);

	spfs_entity_dir_add_child(track_dir,
			spfs_entity_dir_create("artists",
				&(struct spfs_dir_ops) {.readdir = artists_readdir})
			);

	spfs_entity_dir_add_child(track_browse_dir, track_dir);
	return track_dir;
}


