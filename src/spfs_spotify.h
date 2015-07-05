#ifndef SPFS_SPOTIFY_H
#define SPFS_SPOTIFY_H

#include <libspotify/api.h>

sp_session * spotify_session_init(const char *username, const char *password, const char *blob);
void spotify_session_destroy(sp_session *);
void spotify_threads_init();
void spotify_threads_destroy();
sp_connectionstate spotify_connectionstate();
const char * spotify_connectionstate_str();
char ** spotify_artist_search(sp_session * session, const char *query);
void spotify_artist_search_destroy(char **artists);


#endif /* SPFS_SPOTIFY_H */
