#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <glib.h>
#include <libspotify/api.h>
#include "spfs_spotify.h"
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


void *spfs_init(struct fuse_conn_info *conn)
{
	sp_session *session = NULL;
	struct fuse_context *context = fuse_get_context();
	struct spotifile_config *conf= (struct spotifile_config *) context->private_data;
	g_info("%s initialising ...", application_name);
	session = spotify_session_init(conf->spotify_username, conf->spotify_password, NULL);
	spotify_threads_init();
	g_info("%s initialised", application_name);
	return session;

}

void spfs_destroy(void *blob)
{
	sp_session *session = (sp_session *)blob;
	spotify_session_destroy(session);
	spotify_threads_destroy();
	g_info("%s destroyed", application_name);
}

void spfs_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {
	FILE *f = (FILE *)user_data;
	fprintf(f, "%s\n", message);
	fflush(f);
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

	g_log_set_default_handler(spfs_log_handler, stdout);

	if (conf.spotify_username != NULL && !conf.spotify_password)
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
