#ifndef SPFS_SPOTIFY_H
#define SPFS_SPOTIFY_H

#include <glib.h>
#include <libspotify/api.h>

sp_session * spotify_session_init(const char *username, const char *password, const char *blob);
void spotify_session_destroy(sp_session *);
void spotify_threads_init(sp_session *);
void spotify_threads_destroy();
sp_connectionstate spotify_connectionstate();
const char * spotify_connectionstate_str(sp_connectionstate connectionstate);
const char * spotify_track_offline_status_str(sp_track_offline_status);

/* higher level functions */
GArray *spotify_get_playlists(sp_session *session);
GArray *spotify_get_playlist_tracks(sp_playlist *playlist);
GArray *spotify_get_artistbrowse_albums(sp_artistbrowse *artistbrowse);
GArray *spotify_get_artistbrowse_portraits(sp_artistbrowse *artistbrowse);

/*misc. playback*/
bool spotify_play_track(sp_session *session, sp_track *track);
void spotify_seek_track(sp_session *session, int offset);
int spotify_get_track_info(int *channels, int *rate);
size_t spotify_get_audio(char *buf, size_t size, size_t *nsamples);
bool spotify_is_playing(void);

/*artists*/
gchar * spotify_artist_name(sp_artist * artist);

/*albums*/
gchar * spotify_album_name(sp_album * album);
const byte * spotify_album_cover(sp_album * album, sp_image_size size);

/*artist browse*/
sp_artistbrowse *spotify_artistbrowse_create(sp_session *session, sp_artist *artist);
sp_artist * spotify_artistbrowse_artist(sp_artistbrowse *artistbrowse);
char * spotify_artistbrowse_biography(sp_artistbrowse *artistbrowse);

/*tracks*/
gchar * spotify_track_name(sp_track *track);
int spotify_track_duration(sp_track *track);
int spotify_track_index(sp_track *track);
int spotify_track_disc(sp_track *track);
int spotify_track_popularity(sp_track *track);
int spotify_track_num_artists(sp_track *track);
bool spotify_track_is_starred(sp_session *session, sp_track *track);
bool spotify_track_is_local(sp_session *session, sp_track *track);
bool spotify_track_is_autolinked(sp_session *session, sp_track *track);
sp_track_offline_status spotify_track_offline_get_status(sp_track *track);
sp_artist *spotify_track_artist(sp_track *track, int index);

/* images */
sp_image * spotify_image_create(sp_session *session, const byte *id);
void * spotify_image_data(sp_image *image, size_t *size);

/*playlists*/
gchar * spotify_playlist_name(sp_playlist *playlist);
int spotify_playlist_num_tracks(sp_playlist *playlist);
int spotify_playlist_track_create_time(sp_playlist *playlist, int index);
sp_track *spotify_playlist_track(sp_playlist *playlist, int index);

/*playlist containers*/
int spotify_playlistcontainer_num_playlists(sp_playlistcontainer *playlistcontainer);
sp_playlist * spotify_playlistcontainer_playlist(sp_playlistcontainer *playlistcontainer, int index);

/*session*/
sp_playlistcontainer * spotify_session_playlistcontainer(sp_session *session);

/*links*/
sp_link * spotify_link_create_from_artist(sp_artist *artist);
sp_link * spotify_link_create_from_track(sp_track *track);
sp_link * spotify_link_create_from_album(sp_album *album);
sp_link * spotify_link_create_from_string(const char *str);
int spotify_link_as_string(sp_link *link, char *buf, int buffer_size);
sp_artist * spotify_link_as_artist(sp_link *link);


#endif /* SPFS_SPOTIFY_H */
