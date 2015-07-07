#ifndef SPFS_SPOTIFY_H
#define SPFS_SPOTIFY_H

#include <glib.h>
#include <libspotify/api.h>

sp_session * spotify_session_init(const char *username, const char *password, const char *blob);
void spotify_session_destroy(sp_session *);
void spotify_threads_init(sp_session *);
void spotify_threads_destroy();
sp_connectionstate spotify_connectionstate();
const char * spotify_connectionstate_str();
GSList * spotify_artist_search(sp_session * session, const char *query);
const gchar * spotify_artist_name(sp_artist * artist);
sp_link * spotify_link_create_from_artist(sp_artist *artist);
sp_link * spotify_link_create_from_string(const char *str);
sp_link * spotify_link_create_from_string(const char *str);
int spotify_link_as_string(sp_link *link, char *buf, int buffer_size);
sp_artist * spotify_link_as_artist(sp_link *link);


#endif /* SPFS_SPOTIFY_H */
