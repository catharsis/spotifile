#define FUSE_USE_VERSION 26
#define CONN_STATE_SIZE 128
#define SPOTIFY_USERNAME_MAXLEN 128
#define SPOTIFILE_VERSION "v0.0.1"
#include <fuse.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>

#define handle_error_en(en, msg) \
	do { errno = en; g_error("%s (%s)", msg, strerror(errno));} while(0)

#define handle_error(msg) \
	do { g_error(msg); } while(0)

static const char application_name[] = "spotifile";
/* fuse stuff */
struct fuse_operations spfs_operations;
