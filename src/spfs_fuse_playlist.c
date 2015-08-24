#include <libspotify/api.h>
#include "spfs_spotify.h"
#include "spfs_fuse_playlist.h"
#include "spfs_fuse_track.h"
#include "spfs_fuse_audiofile.h"
#include "spfs_format.h"
/*readdir for a single playlist directory*/
static int playlist_dir_meta_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {

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
			track_link->mtime = track_create_time;
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

static int playlist_dir_music_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {

	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	sp_playlist *pl = (sp_playlist *)e->auxdata;
	GArray *tracks = spotify_get_playlist_tracks(pl);
	for (guint i = 0; i < tracks->len; ++i) {
		sp_track *track = g_array_index(tracks, sp_track *, i);
		gchar *trackname = spotify_track_name(track);
		GString *formatted_trackname = spfs_format_playlist_track_expand(SPFS_DATA->playlist_track_format, track, i+1);
		g_free(trackname);

		time_t track_create_time = spotify_playlist_track_create_time(pl, i);
		if (!spfs_entity_dir_has_child(e->e.dir, formatted_trackname->str)) {
			spfs_entity *wav_file= create_track_wav_file(formatted_trackname->str, track);
			wav_file->mtime = track_create_time;
			spfs_entity_dir_add_child(e, wav_file);
		}
		g_string_free(formatted_trackname, TRUE);
	}
	g_array_free(tracks, false);
	return 0;
}

/*readdir for the "music" playlists directory*/
int playlists_music_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {
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
int playlists_meta_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {
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
