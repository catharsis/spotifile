#define FUSE_USE_VERSION 26
#define CONN_STATE_SIZE 128
#define SPOTIFY_USERNAME_MAXLEN 128
#include <libspotify/api.h>
#include <fuse.h>
#include <stdint.h>
#include <stdlib.h>

sp_session *spotify_session;
