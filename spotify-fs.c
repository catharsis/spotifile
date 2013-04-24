#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include "spotify-fs.h"
struct spotifile_config {
	char *spotify_username;
	char *spotify_password;
	bool remember_me;
};

enum {
	KEY_HELP,
	KEY_VERSION,
};

FILE *logfile;

#define SPFS_OPT(t, p, v) {t, offsetof(struct spotifile_config, p), v }

static struct fuse_opt spfs_opts[] = {
	SPFS_OPT("username=%s",		spotify_username, 0),
	SPFS_OPT("password=%s",		spotify_password, 0),
	SPFS_OPT("--rememberme=true",	remember_me, 1),
	SPFS_OPT("--rememberme=false",	remember_me, 0),
	FUSE_OPT_KEY("-V",			KEY_VERSION),
	FUSE_OPT_KEY("--version",	KEY_VERSION),
	FUSE_OPT_KEY("-h",			KEY_HELP),
	FUSE_OPT_KEY("--help",		KEY_HELP),
	FUSE_OPT_END
};

static int spfs_opt_process(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	switch(key) {
		case KEY_HELP:
			fprintf(stderr,
					"usage: %s mountpoint [options]\n"
					"\n"
					"general options:\n"
					"		-o opt,[opt...]	mount options\n"
					"		-h	--help		this cruft\n"
					"		-V	--version	print version\n"
					"\n"
					"spotifile options:\n"
					"		-o username=STRING\n"
					"		-o password=STRING\n"
					"		--rememberme=BOOL\n"
					, outargs->argv[0]);
			exit(1);
		case KEY_VERSION:
			fprintf(stderr, "spotifile version %s\n", SPOTIFILE_VERSION);
			exit(0);
	}
	return 1;
}
void spfs_log_init(const char *log_path)
{
	logfile = fopen("spotifile.log", "w");
	if (logfile == NULL) {
		handle_error("fopen");
	}
}

void spfs_log_close()
{
	if(fclose(logfile) != 0)
		handle_error("fclose");
}

/*thread safe, but non-reentrant */
void spfs_log(const char *format, ...)
{
	static pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;
	char *lformat = strdup(format);
	lformat = realloc(lformat, strlen(format) + 1);
	strncat(lformat, "\n", strlen(format) + 1);
	va_list ap;
	int ret = 0;
	va_start(ap, format);
	MUTEX_LOCK(ret, &logmutex);
	vfprintf(logfile, lformat, ap);
	fflush(logfile);
	MUTEX_UNLOCK(ret, &logmutex);
	va_end(ap);
	free(lformat);
}

void spfs_log_errno(const char *topic) {
	char *local_topic = strdup(topic);
	char *message = strerror(errno);
	size_t totlen = 0;
	totlen = strlen(topic) + strlen(message);
	local_topic = realloc(local_topic, totlen);
	strncat(local_topic, message, totlen);
	spfs_log(local_topic);
	free(local_topic);
}
void *spfs_init(struct fuse_conn_info *conn)
{
	struct fuse_context *context = fuse_get_context();
	struct spotifile_config *conf= (struct spotifile_config *) context->private_data;
	spfs_log("%s initialising ...", application_name);
	spotify_session_init(conf->spotify_username, conf->spotify_password, NULL);
	spotify_threads_init();
	spfs_log("%s initialised", application_name);
	return NULL;

}

void spfs_destroy(void *init_retval)
{
	spotify_threads_destroy();
	spotify_session_destroy();
	spfs_log("%s destroyed", application_name);
	spfs_log_close();
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int retval = 0;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct spotifile_config conf;
	memset(&conf, 0, sizeof(conf));
	if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running %s as root is not a good idea\n", application_name);
		return 1;
	}

	fuse_opt_parse(&args, &conf, spfs_opts, spfs_opt_process);

	spfs_log_init("./spotifile.log");
	if (!conf.spotify_password)
	{
		if((conf.spotify_password = getpass("spotify password:")) == NULL)
			handle_error("getpass");
	}
	spfs_operations.init = spfs_init;
	spfs_operations.destroy = spfs_destroy;
	retval = fuse_main(args.argc, args.argv, &spfs_operations, &conf);
	if (retval != 0) {
		fprintf(stderr, "Error initialising spotifile\n");
	}
	return retval;
}
