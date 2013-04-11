#define FUSE_USE_VERSION 26
#define CONN_STATE_SIZE 128
#define SPOTIFY_USERNAME_MAXLEN 128
#include <libspotify/api.h>
#include <fuse.h>
#include <stdint.h>
#include <stdlib.h>

static const char application_name[] = "spotifile";
sp_session *spotify_session;
int spotify_session_init(const char *username, const char *password, const char *blob);

struct fuse_operations spfs_operations;
