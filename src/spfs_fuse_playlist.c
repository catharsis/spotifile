#include <libspotify/api.h>
#include "spfs_spotify.h"
#include "spfs_fuse_playlist.h"
#include "spfs_fuse_track.h"
/*readdir for a single playlist directory*/
int playlist_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {

	spfs_entity *e = (spfs_entity *)fi->fh;
	sp_playlist *pl = (sp_playlist *)e->auxdata;

	int num_tracks = spotify_playlist_num_tracks(pl);
	for (int i = 0; i < num_tracks; ++i) {
		sp_track *track = spotify_playlist_track(pl, i);
		const gchar *trackname = spotify_track_name(track);
		spfs_entity *track_browse_dir = create_track_browse_dir(track);
		if (!spfs_entity_dir_has_child(e->e.dir, trackname)) {
			spfs_entity *track_link = spfs_entity_link_create(trackname, NULL);
			track_link->mtime = spotify_playlist_track_create_time(pl, i);
			spfs_entity_dir_add_child(e, track_link);
			gchar *rpath = relpath(e, track_browse_dir);
			spfs_entity_link_set_target(track_link, rpath);
			g_free(rpath);

		}
	}
	return 0;
}

/*readdir for the "root" playlists directory*/
int playlists_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {
	sp_session *session = SPFS_SP_SESSION;
	g_return_val_if_fail(session != NULL, 0);
	spfs_entity *playlists_dir = (spfs_entity *)fi->fh;

	sp_playlistcontainer *playlistcontainer = spotify_session_playlistcontainer(session);
	int num_playlists = spotify_playlistcontainer_num_playlists(playlistcontainer);
	for (int i = 0; i < num_playlists; ++i) {
		sp_playlist * pl = spotify_playlistcontainer_playlist(playlistcontainer, i);
		const gchar *name = spotify_playlist_name(pl);
		if (!spfs_entity_dir_has_child(playlists_dir->e.dir, name)) {
			spfs_entity * pld = spfs_entity_dir_create(name,
					playlist_dir_readdir);
			pld->auxdata = pl;
			spfs_entity_dir_add_child(playlists_dir, pld);
		}
	}
	return 0;
}
