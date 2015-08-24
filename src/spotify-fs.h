#ifndef SPOTIFY_FS_H
#define SPOTIFY_FS_H
#define CONN_STATE_SIZE 128

#ifndef SPOTIFILE_VERSION
#error "Undefined version?"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <glib.h>

/* Time (in seconds) to wait for callbacks to complete (synchronous calls)*/
#define SPFS_CB_TIMEOUT_S 30

#define handle_error_en(en, msg) \
	do { errno = en; g_error("%s (%s)", msg, strerror(errno));} while(0)

#define handle_error(msg) \
	do { g_error(msg); } while(0)

struct spotifile_config {
	char *spotify_username;
	char *spotify_password;
	char *config_file;
	gchar *playlist_track_format;
	int foreground;
	int debug;
};

static const char application_name[] = "spotifile";
#endif /* SPOTIFY_FS_H */
