#include <libspotify/api.h>
#include "spfs_spotify.h"
#include "spfs_fuse_playlist.h"
#include "spfs_fuse_track.h"
#include "xspf.h"
/*readdir for a single playlist directory*/
static int playlist_dir_meta_readdir(struct fuse_file_info *fi, void *buf, fuse_fill_dir_t filler, off_t offset) {

	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	sp_playlist *pl = (sp_playlist *)e->auxdata;

	GArray *tracks = spotify_get_playlist_tracks(pl);
	for (guint i = 0; i < tracks->len; ++i) {
		sp_track *track = g_array_index(tracks, sp_track *, i);
		gchar *trackname = spotify_track_name(track);
		spfs_entity *track_browse_dir = create_track_browse_dir(track);
		time_t track_create_time = spotify_playlist_track_create_time(pl, i);
		if (!spfs_entity_dir_has_child(e->e.dir, trackname)) {
			spfs_entity *track_link = spfs_entity_link_create(trackname, NULL);
			spfs_entity_set_stat_times(track_link, &(struct stat_times){.mtime = track_create_time});
			spfs_entity_dir_add_child(e, track_link);
			gchar *rpath = relpath(e, track_browse_dir);
			spfs_entity_link_set_target(track_link, rpath);
			g_free(rpath);
		}

		g_free(trackname);
	}
	g_array_free(tracks, false);
	return 0;
}

static xspf * populate_xspf_from_sp_track(xspf *x, sp_track *track) {
	x = xspf_track_set_duration(x, spotify_track_duration(track));
	gchar *name = spotify_track_name(track);
	x = xspf_track_set_title(x, name);
	g_free(name);

	sp_album *album = spotify_track_album(track);
	gchar *album_name = spotify_album_name(album);
	xspf_track_set_album(x, album_name);
	g_free(album_name);

	GArray *artists = spotify_get_track_artists(track);
	for (guint i = 0; i < artists->len; ++i) {
		sp_artist *artist = g_array_index(artists, sp_artist *, i);
		gchar * artist_name = spotify_artist_name(artist);
		x = xspf_track_set_creator(x, artist_name);
		g_free(artist_name);
	}
	g_array_free(artists, false);
	return x;
}

static int xspf_read(struct fuse_file_info *fi, char *buf, size_t size, off_t offset) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	GHashTableIter iter;
	gpointer k, v;
	xspf *x = xspf_new();
	x = xspf_begin_playlist(x);
	x = xspf_begin_tracklist(x);
	g_hash_table_iter_init(&iter, e->parent->e.dir->children);
	while (g_hash_table_iter_next(&iter, &k, &v)) {
		if (0 == g_strcmp0("playlist.xspf", k)) continue;
		x = xspf_begin_track(x);
		sp_track *track = ((spfs_entity *)v)->auxdata;
		x = xspf_track_set_location(x, k);
		x = populate_xspf_from_sp_track(x, track);
		x = xspf_end_track(x);
	}
	x = xspf_end_tracklist(x);
	x = xspf_end_playlist(x);

	gchar *xspf_str = xspf_free(x, false);
	READ_OFFSET(xspf_str, buf, size, offset);
	g_free(xspf_str);
	return size;
}

static int playlist_dir_music_readdir(struct fuse_file_info *fi, void *buf, fuse_fill_dir_t filler, off_t offset) {

	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	sp_playlist *pl = (sp_playlist *)e->auxdata;
	GArray *tracks = spotify_get_playlist_tracks(pl);
	for (guint i = 0; i < tracks->len; ++i) {
		sp_track *track = g_array_index(tracks, sp_track *, i);
		gchar *trackname = spotify_track_name(track);
		gchar *formatted_trackname = g_strdup_printf("%.2d - %s.wav", i, trackname);
		g_free(trackname);

		time_t track_create_time = spotify_playlist_track_create_time(pl, i);
		create_wav_track_link_in_directory(e, formatted_trackname, track, &(struct stat_times){.mtime = track_create_time});
		g_free(formatted_trackname);
	}

	if (!spfs_entity_dir_has_child(e->e.dir, "playlist.xspf")) {
		spfs_entity *xspf_playlist = spfs_entity_file_create("playlist.xspf",
				&(struct spfs_file_ops){
				.read = xspf_read
				});
		spfs_entity_dir_add_child(e, xspf_playlist);
	}
	g_array_free(tracks, false);
	return 0;
}

/*readdir for the "music" playlists directory*/
int playlists_music_dir_readdir(struct fuse_file_info *fi, void *buf,
		fuse_fill_dir_t filler, off_t offset) {
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	spfs_entity *playlists_dir = SPFS_FH2ENT(fi->fh);
	GArray *playlists = spotify_get_playlists(session);
	for (guint i = 0; i < playlists->len; ++i) {
		sp_playlist * pl = g_array_index(playlists, sp_playlist *, i);
		gchar *name = spotify_playlist_name(pl);
		if (!spfs_entity_dir_has_child(playlists_dir->e.dir, name)) {
			spfs_entity * pld = spfs_entity_dir_create(name,
					&(struct spfs_dir_ops) {.readdir = playlist_dir_music_readdir});
			pld->auxdata = pl;
			spfs_entity_dir_add_child(playlists_dir, pld);
		}

		g_free(name);
	}
	g_array_free(playlists, false);

	return 0;
}

/*readdir for the "meta" playlists directory*/
int playlists_meta_dir_readdir(struct fuse_file_info *fi, void *buf,
		fuse_fill_dir_t filler, off_t offset) {
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	spfs_entity *playlists_dir = SPFS_FH2ENT(fi->fh);
	GArray *playlists = spotify_get_playlists(session);
	for (guint i = 0; i < playlists->len; ++i) {
		sp_playlist * pl = g_array_index(playlists, sp_playlist *, i);
		gchar *name = spotify_playlist_name(pl);
		if (!spfs_entity_dir_has_child(playlists_dir->e.dir, name)) {
			spfs_entity * pld = spfs_entity_dir_create(name,
					&(struct spfs_dir_ops) {.readdir = playlist_dir_meta_readdir});
			pld->auxdata = pl;
			spfs_entity_dir_add_child(playlists_dir, pld);
		}

		g_free(name);
	}
	g_array_free(playlists, false);
	return 0;
}
