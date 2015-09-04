#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include <glib.h>
#include <libspotify/api.h>
#include "spotify-fs.h"
#include "spfs_spotify.h"
#include "spfs_fuse.h"
#include "spfs_format.h"

enum {
	KEY_HELP,
	KEY_VERSION,
};

#define SPFS_OPT(t, p, v) {t, offsetof(struct spotifile_config, p), v }

static struct fuse_opt spfs_opts[] = {
	SPFS_OPT("username=%s",		spotify_username, 0),
	SPFS_OPT("password=%s",		spotify_password, 0),
	SPFS_OPT("-c %s",	config_file, 0),
	SPFS_OPT("-f",			foreground, 1),
	SPFS_OPT("-d",			debug, 1),
	FUSE_OPT_KEY("-f",			FUSE_OPT_KEY_KEEP),
	FUSE_OPT_KEY("-d",			FUSE_OPT_KEY_KEEP),
	FUSE_OPT_KEY("-V",			KEY_VERSION),
	FUSE_OPT_KEY("--version",	KEY_VERSION),
	FUSE_OPT_KEY("-h",			KEY_HELP),
	FUSE_OPT_KEY("--help",		KEY_HELP),
	FUSE_OPT_END
};

struct spotifile_log_options{
	FILE *fp;
	bool debug;
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
					"		-h, --help	this cruft\n"
					"		-V, --version	print version\n"
					"		-d		run in debug mode (implies -f)\n"
					"		-f		run in foreground\n"
					"		-s		run (FUSE) in single-threaded mode\n"
					"		-c configfile	path to configuration file\n"
					"\n"
					"spotify options:\n"
					"		-o username=STRING\n"
					"		-o password=STRING\n"
					, outargs->argv[0]);
			exit(1);
		case KEY_VERSION:
			fprintf(stderr, "spotifile version " SPOTIFILE_VERSION "\n");
			exit(0);
	}
	return 1;
}

static int log_level_to_syslog_level(GLogLevelFlags log_level) {
	switch (log_level) {
		case G_LOG_LEVEL_ERROR:
			return LOG_ERR;
			break;
		case G_LOG_LEVEL_CRITICAL:
			return LOG_CRIT;
			break;
		case G_LOG_LEVEL_WARNING:
			return LOG_WARNING;
			break;
		case G_LOG_LEVEL_MESSAGE:
			return LOG_NOTICE;
			break;
		case G_LOG_LEVEL_INFO:
			return LOG_INFO;
			break;
		case G_LOG_LEVEL_DEBUG:
			return LOG_DEBUG;
			break;
		default:
			g_warn_if_reached();
			return LOG_DEBUG;
			break;

	}
}

static char log_level_ind(GLogLevelFlags log_level) {
	char lvl;
	switch (log_level & ~G_LOG_FLAG_FATAL & ~G_LOG_FLAG_RECURSION) {
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
			lvl = '?';
			break;

	}
	return lvl;
}

void spfs_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {

	struct spotifile_log_options *options = (struct spotifile_log_options *)user_data;
	FILE *f = options->fp;

	if (log_level == G_LOG_LEVEL_DEBUG && !(options->debug))
		return;

	if (options->fp == NULL) {
		syslog( log_level_to_syslog_level(log_level),
				"%s",
				message
				);
	}
	else {
		GDateTime *log_time = g_date_time_new_now_local();
		gchar *log_time_str = g_date_time_format(log_time, "%F %T %Z");
		fprintf(f, "[%c @ %s] %s: %s\n", log_level_ind(log_level), log_time_str, application_name, message);
		fflush(f);
		g_free(log_time_str);
		g_date_time_unref(log_time);
	}
}

static void load_configuration(struct spotifile_config *config)
{
	GKeyFile * config_file = g_key_file_new();
	gchar *config_path = NULL;
	if (!config->config_file) {
		gchar *config_path = g_build_filename(g_get_user_config_dir(), "spotifile", "spotifile.conf", NULL);
		g_debug("No config file specified, falling back to %s", config_path);
		config->config_file = config_path;
	}

	g_debug("Loading configuration from %s", config->config_file);
	GError *err = NULL;
	if (!g_key_file_load_from_file(config_file, config->config_file, G_KEY_FILE_NONE, &err)) {
		g_warning("Could not load configuration from %s: %s", config->config_file, err->message);
		g_error_free(err);
		err = NULL;
		goto out;
	}

	// command line options override config file ones
	if (!config->spotify_username) {
		config->spotify_username = g_key_file_get_string(config_file, "spotify", "username", &err);
		if (!config->spotify_username) {
			g_message("No Spotify username specified: %s", err->message);
			g_error_free(err);
			err = NULL;
		}
	}

	if (!config->spotify_password) {
		err = NULL;
		config->spotify_password = g_key_file_get_string(config_file, "spotify", "password", &err);
		if (!config->spotify_password) {
			g_message("No Spotify password specified: %s", err->message);
			g_error_free(err);
			err = NULL;
		}
	}

	err = NULL;
	config->playlist_track_format = g_key_file_get_string(config_file, "playlists", "track_format", &err);
	if (!config->playlist_track_format || !spfs_format_isvalid(config->playlist_track_format)) {
		if (err) {
			g_message("No playlist track format specified (%s), using default", err->message);
			g_error_free(err);
			err = NULL;
		}
		else {
			g_warning("Invalid playlist track format (%s), using default", config->playlist_track_format);
		}

		config->playlist_track_format = g_strdup("%i - %a - %t");
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
	struct spotifile_log_options *log_options;

	memset(&conf, 0, sizeof(conf));
	log_options = g_new0(struct spotifile_log_options, 1);

	if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running %s as root is not a good idea\n", application_name);
		return 1;
	}

	fuse_opt_parse(&args, &conf, spfs_opts, spfs_opt_process);
	fuse_opt_add_arg(&args, "-oentry_timeout=0");

	if (conf.debug)
		conf.foreground = 1;

	log_options->debug = (bool) conf.debug;
	if (conf.foreground)
		log_options->fp = stdout;
	else
		log_options->fp = NULL;

	g_log_set_default_handler(spfs_log_handler, log_options);

	load_configuration(&conf);

	if (conf.spotify_username != NULL && !conf.spotify_password && isatty(fileno(stdin)))
	{
		if((conf.spotify_password = getpass("spotify password:")) == NULL)
			handle_error("getpass");
	}

	if (!conf.spotify_username) {
		g_warning("Missing Spotify username");
		exit(1);
	}

	if (!conf.spotify_password) {
		g_warning("Missing Spotify password");
		exit(1);
	}

	struct fuse_operations fuse_ops = spfs_get_fuse_operations();
	retval = fuse_main(args.argc, args.argv, &fuse_ops, &conf);
	fuse_opt_free_args(&args);
	if (retval != 0) {
		fprintf(stderr, "Error initialising spotifile\n");
	}
	return retval;
}
