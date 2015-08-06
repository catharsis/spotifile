#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <glib.h>
#include <libspotify/api.h>
#include "spotify-fs.h"
#include "spfs_spotify.h"
#include "spfs_fuse.h"
enum {
	KEY_HELP,
	KEY_VERSION,
};

#define SPFS_OPT(t, p, v) {t, offsetof(struct spotifile_config, p), v }

static struct fuse_opt spfs_opts[] = {
	SPFS_OPT("username=%s",		spotify_username, 0),
	SPFS_OPT("password=%s",		spotify_password, 0),
	SPFS_OPT("-c %s",	config_file, 0),
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
					"		-c configfile		path to configuration file\n"
					"\n"
					"spotify options:\n"
					"		-o username=STRING\n"
					"		-o password=STRING\n"
					, outargs->argv[0]);
			exit(1);
		case KEY_VERSION:
			fprintf(stderr, "spotifile version %s\n", SPOTIFILE_VERSION);
			exit(0);
	}
	return 1;
}

void spfs_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {
	FILE *f = (FILE *)user_data;
	char lvl;
	switch (log_level) {
		case G_LOG_LEVEL_ERROR:
			lvl = 'E';
			break;
		case G_LOG_LEVEL_CRITICAL:
			lvl = 'C';
			break;
		case G_LOG_LEVEL_WARNING:
			lvl = 'W';
			break;
		case G_LOG_LEVEL_MESSAGE:
			lvl = 'M';
			break;
		case G_LOG_LEVEL_INFO:
			lvl = 'I';
			break;
		case G_LOG_LEVEL_DEBUG:
			lvl = 'D';
			break;
		default:
			g_warn_if_reached();
			lvl = '?';
			break;

	}
	fprintf(f, "[%c] %s: %s\n", lvl, application_name, message);
	fflush(f);
}

static void load_configuration(struct spotifile_config *config)
{
	GKeyFile * config_file = g_key_file_new();
	gchar *config_path = NULL;
	if (!config->config_file) {
		gchar *config_path = g_build_filename(g_get_user_config_dir(), "spotifile.conf", NULL);
		g_debug("No config file specified, falling back to %s", config_path);
		config->config_file = config_path;
	}

	g_debug("Loading configuration from %s", config->config_file);
	GError *err = NULL;
	if (!g_key_file_load_from_file(config_file, config->config_file, G_KEY_FILE_NONE, &err)) {
		g_warning("Could not load configuration from %s: %s", config->config_file, err->message);
		goto out;
	}

	// command line options override config file ones
	if (!config->spotify_username) {
		config->spotify_username = g_key_file_get_string(config_file, "spotify", "username", &err);
		if (!config->spotify_username) {
			g_message("No Spotify username specified: %s", err->message);
		}
	}

	if (!config->spotify_password) {
		config->spotify_password = g_key_file_get_string(config_file, "spotify", "password", &err);
		if (!config->spotify_password) {
			g_message("No Spotify password specified: %s", err->message);
		}
	}

out:
	g_key_file_free(config_file);
	g_free(config_path);
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
	fuse_opt_add_arg(&args, "-oentry_timeout=0");
	g_log_set_default_handler(spfs_log_handler, stdout);

	load_configuration(&conf);

	if (conf.spotify_username != NULL && !conf.spotify_password)
	{
		if((conf.spotify_password = getpass("spotify password:")) == NULL)
			handle_error("getpass");
	}

	struct fuse_operations fuse_ops = spfs_get_fuse_operations();
	retval = fuse_main(args.argc, args.argv, &fuse_ops, &conf);
	if (retval != 0) {
		fprintf(stderr, "Error initialising spotifile\n");
	}
	return retval;
}
