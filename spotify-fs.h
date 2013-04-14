#define FUSE_USE_VERSION 26
#define CONN_STATE_SIZE 128
#define SPOTIFY_USERNAME_MAXLEN 128
#include <libspotify/api.h>
#include <fuse.h>
#include <stdint.h>
#include <stdlib.h>

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while(0)

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while(0)

static const char application_name[] = "spotifile";

void spfs_log(const char *message);

/*spotify stuff*/
int spotify_session_init(const char *username, const char *password, const char *blob);
void * spotify_thread_start_routine(void *arg);
char * spotify_connectionstate_str();

/* fuse stuff */
struct fuse_operations spfs_operations;
