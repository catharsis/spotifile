#include "spfs_fuse_search.h"
#include "spfs_fuse_track.h"
#include "spfs_spotify.h"
#include <glib.h>
static int search_readdir(struct fuse_file_info *fi, void *buf, fuse_fill_dir_t filler, off_t offset) {
	return 0;
}

void track_search_complete_cb(sp_search *search, void *entity) {
	int num_tracks = spotify_search_num_tracks(search);
	spfs_entity *e = entity;
	for (int i = 0; i < num_tracks; ++i) {
		sp_track * track = spotify_search_track(search, i);
		gchar *trackname = spotify_track_name(track);
		gchar *artistname = spotify_artist_name(spotify_track_artist(track, 0));
		gchar *formatted_trackname = g_strdup_printf("%s - %s.wav", artistname, trackname);
		g_free(artistname);
		g_free(trackname);
		create_wav_track_link_in_directory(e, formatted_trackname, track, NULL);
		g_free(formatted_trackname);
	}
}

int search_dir_mkdir(spfs_entity *parent, const char *dirname, mode_t mode) {
	if (spfs_entity_dir_has_child(parent->e.dir, dirname)) {
		return -EEXIST;
	}

	spfs_entity *e = spfs_entity_dir_create(dirname,
				&(struct spfs_dir_ops){
				.mkdir = NULL,
				.readdir = search_readdir
				});
	e->auxdata = spotify_search_create_track_search(SPFS_SP_SESSION,
			dirname, /* query */
			0, 100, /*track offset & count - completely arbitrarily chosen*/
			track_search_complete_cb, e
			);

	spfs_entity_dir_add_child(parent, e);
	return 0;
}
