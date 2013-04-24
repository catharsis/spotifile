#define FUSE_USE_VERSION 26
#define CONN_STATE_SIZE 128
#define SPOTIFY_USERNAME_MAXLEN 128
#define SPOTIFILE_VERSION "v0.0.1"
#include <libspotify/api.h>
#include <fuse.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

void spfs_log(const char *format, ...);
void spfs_log_errno(const char *topic);

#define handle_error_en(en, msg) \
	do { errno = en; spfs_log_errno(msg); exit(EXIT_FAILURE); } while(0)

#define handle_error(msg) \
	do { spfs_log_errno(msg); exit(EXIT_FAILURE); } while(0)

#define MUTEX_LOCK(mx_lock_ret, m) \
	do { \
		if ((mx_lock_ret = pthread_mutex_lock(m)) != 0) \
			handle_error_en(mx_lock_ret, "pthread_mutex_lock"); \
	} while (0)

#define MUTEX_UNLOCK(mx_unlock_ret, m) \
	do { \
		if ((mx_unlock_ret = pthread_mutex_unlock(m)) != 0) \
			handle_error_en(mx_unlock_ret, "pthread_mutex_unlock"); \
	} while (0)


static const char application_name[] = "spotifile";
extern time_t g_logged_in_at;


/*spotify stuff*/
void spotify_session_init(const char *username, const char *password, const char *blob);
void spotify_session_destroy();
void spotify_threads_init();
void spotify_threads_destroy();
char * spotify_connectionstate_str();

/* fuse stuff */
struct fuse_operations spfs_operations;
