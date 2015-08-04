#include "spotify-fs.h"
#include "spfs_spotify.h"
#include "spfs_audio.h"
#include "spfs_appkey.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
time_t g_logged_in_at = (time_t) -1;

/* thread globals */
static bool g_main_thread_do_notify = false;
static bool g_running = false;
static bool g_playback_done = false;
static spfs_audio_playback *g_playback;
static GMutex g_spotify_notify_mutex;
static GMutex g_spotify_api_mutex;
static GCond g_spotify_notify_cond;
static GCond g_spotify_data_available;
static GThread *spotify_thread;

/*foward declarations*/
void * spotify_thread_start(void *arg);

#define STRINGIFY(x) #x
#define SPFS_WAIT_ON_FUNC(_Type) \
	static bool wait_on_ ## _Type (sp_ ## _Type *_Type) { \
		if (!_Type) return false; \
		gint64 end_time = g_get_monotonic_time() + SPFS_CB_TIMEOUT_S * G_TIME_SPAN_SECOND; \
		while (! sp_ ## _Type ## _is_loaded ( _Type )) { \
			g_debug("waiting for " STRINGIFY(_Type ) " to load"); \
			if (!g_cond_wait_until(&g_spotify_data_available, &g_spotify_api_mutex, end_time)) \
			{ \
				g_debug(STRINGIFY(_Type ) " still not loaded...giving up"); \
				return false; \
			} \
		} \
		return true; \
	}

#define SPFS_SPOTIFY_SESSION_API_FUNC(_Ret, _Type, _Field) \
	_Ret spotify_ ## _Type ## _ ## _Field (sp_session *session, sp_ ## _Type * _Type) { \
	_Ret _Field; \
	g_mutex_lock(&g_spotify_api_mutex); \
	if (!wait_on_ ## _Type( _Type )) { \
		g_warning(STRINGIFY(_Type) " never loaded, unreliable " STRINGIFY(_Field) " information"); \
	} \
	_Field = sp_ ## _Type ## _ ## _Field (session, _Type); \
	g_mutex_unlock(&g_spotify_api_mutex); \
	return _Field; \
	}

#define SPFS_SPOTIFY_API_FUNC2(_Ret, _Type, _Field, _Type2, _Field2) \
	_Ret spotify_ ## _Type ## _ ## _Field (sp_ ## _Type * _Type, _Type2 _Field2) { \
	_Ret _Field; \
	g_mutex_lock(&g_spotify_api_mutex); \
	if (!wait_on_ ## _Type( _Type )) { \
		g_warning(STRINGIFY(_Type) " never loaded, unreliable " STRINGIFY(_Field) " information"); \
	} \
	_Field = sp_ ## _Type ## _ ## _Field (_Type, _Field2); \
	g_mutex_unlock(&g_spotify_api_mutex); \
	return _Field; \
}

#define SPFS_SPOTIFY_API_FUNC(_Ret, _Type, _Field) \
	_Ret spotify_ ## _Type ## _ ## _Field (sp_ ## _Type * _Type) { \
	_Ret _Field; \
	g_mutex_lock(&g_spotify_api_mutex); \
	if (!wait_on_ ## _Type( _Type )) { \
		g_warning(STRINGIFY(_Type) " never loaded, unreliable " STRINGIFY(_Field) " information"); \
	} \
	_Field = sp_ ## _Type ## _ ## _Field (_Type); \
	g_mutex_unlock(&g_spotify_api_mutex); \
	return _Field; \
}


SPFS_WAIT_ON_FUNC(image)
SPFS_WAIT_ON_FUNC(artist)
SPFS_WAIT_ON_FUNC(album)
SPFS_WAIT_ON_FUNC(track)
SPFS_WAIT_ON_FUNC(playlist)
SPFS_WAIT_ON_FUNC(user)
SPFS_WAIT_ON_FUNC(playlistcontainer)
#define wait_on_session(S) (true)
int spotify_login(sp_session * session, const char *username, const char *password, const char *blob) {
	if (username == NULL) {
		char reloginname[256];

		if (sp_session_relogin(session) == SP_ERROR_NO_CREDENTIALS) {
			g_message("no stored credentials");
			return 1;
		}

		sp_session_remembered_user(session, reloginname, sizeof(reloginname));
		g_message("trying to relogin as user %s", reloginname);

	} else {
		g_message("trying to login as %s", username);
		sp_session_login(session, username, password, 1, blob);
	}
	return 0;
}



static void spotify_start_playback(sp_session *session) {
}

static void spotify_end_of_track(sp_session *session) {
	spfs_audio_playback_flush(g_playback);
	g_mutex_lock(&g_playback->mutex);
	g_playback->playing = NULL;
	g_mutex_unlock(&g_playback->mutex);
	g_playback_done = true;
}

static void spotify_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats) {
	g_mutex_lock(&g_playback->mutex);
	stats->samples = g_playback->nsamples;
	stats->stutter = g_playback->stutter;
	g_playback->stutter = 0;
	g_mutex_unlock(&g_playback->mutex);
}

static int spotify_music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames) {
	if (num_frames == 0)
		return 0;

	g_mutex_lock(&(g_playback->mutex));
	if (g_playback->nsamples > (format->sample_rate * 2)) {
		g_mutex_unlock(&g_playback->mutex);
		return 0;
	}
	size_t s = num_frames * sizeof(int16_t) * format->channels;
	spfs_audio *audio = malloc(sizeof(*audio) + s);
	memcpy(audio->samples, frames, s);

	audio->nsamples = num_frames;

	audio->rate = format->sample_rate;
	audio->channels = format->channels;
	g_queue_push_tail(g_playback->queue, audio);
	g_playback->nsamples += num_frames;
	g_cond_signal(&(g_playback->cond));
	g_mutex_unlock(&(g_playback->mutex));

	return num_frames;
}

static void spotify_log_message(sp_session *session, const char *message) {
	/* strip trailing whitespace to keep the log neat */
	gchar *tmp = g_strdup(message);
	g_message("spotify: %s", g_strchomp(tmp));
	g_free(tmp);
}

static void spotify_logged_out(sp_session *session) {
	g_logged_in_at = (time_t) -1;
	g_message("spotify: logged out");
}

static void spotify_logged_in(sp_session *session, sp_error error)
{
	if(SP_ERROR_OK == error) {
		time(&g_logged_in_at);
		g_message("spotify: logged in at %d", (int)g_logged_in_at);
	} else {
		g_message("spotify: logged in failed (%s)", sp_error_message(error));
	}
}

static void spotify_connection_error(sp_session *session, sp_error error)
{
	g_warning("Connection error %d: %s\n", error, sp_error_message(error));
}

static void artist_search_complete_cb(sp_search *result, void *userdata)
{
	g_debug("Artist search complete!");
	g_cond_broadcast(&g_spotify_data_available);
}

static void spotify_notify_main_thread(sp_session *session)
{
	g_mutex_lock(&g_spotify_notify_mutex);
	g_main_thread_do_notify = true;
	g_cond_signal(&g_spotify_notify_cond);
	g_mutex_unlock(&g_spotify_notify_mutex);
}

static sp_session_callbacks spotify_callbacks = {
	.notify_main_thread = spotify_notify_main_thread,
	.logged_in = spotify_logged_in,
	.connection_error = spotify_connection_error,
	.logged_out = spotify_logged_out,
	.log_message = spotify_log_message,
	.music_delivery = spotify_music_delivery,
	.get_audio_buffer_stats = spotify_audio_buffer_stats,
	.start_playback = spotify_start_playback,
	.end_of_track = spotify_end_of_track
};

sp_session * spotify_session_init(const char *username, const char *password, const char *blob)
{
	sp_session_config config;
	sp_error error;
	sp_session *session;
	memset(&config, 0, sizeof(config));
	config.api_version = SPOTIFY_API_VERSION;
	config.cache_location = "/var/tmp/";
	config.settings_location = "/tmp";
	config.application_key = spfs_appkey_get(&config.application_key_size);
	config.callbacks = &spotify_callbacks;

	config.user_agent = application_name;
	error = sp_session_create(&config, &session);
	if ( error != SP_ERROR_OK ) {
		g_warning("failed to create session: %s",
				sp_error_message(error));
		return NULL;
	}
	g_message("spotify session created!");
	if(spotify_login(session, username, password, blob) != 0)
		g_warning("login failed!");

	return session;
}

void spotify_session_destroy(sp_session * session)
{
	sp_session_logout(session);
	free(session);
	g_message("session destroyed");
}

void spotify_threads_init(sp_session *session)
{
	g_mutex_init(&g_spotify_api_mutex);
	g_mutex_init(&g_spotify_notify_mutex);
	g_cond_init(&g_spotify_notify_cond);
	g_cond_init(&g_spotify_data_available);
	g_running = true;
	g_playback = spfs_audio_playback_new();
	spotify_thread = g_thread_new("spotify", spotify_thread_start, session);
}

void spotify_threads_destroy()
{
	g_running = false;
	g_debug("spotify thread cancel request sent");
	g_thread_join(spotify_thread);
	spotify_thread = NULL;
	g_debug("spotify threads destroyed");
}


/* locking accessors */
sp_connectionstate spotify_connectionstate(sp_session * session) {
	sp_connectionstate s;
	g_mutex_lock(&g_spotify_api_mutex);
	s = sp_session_connectionstate(session);
	g_mutex_unlock(&g_spotify_api_mutex);
	return s;
}

bool spotify_is_playing(void) {
	return spfs_audio_playback_is_playing(g_playback);
}

// Returns track information for the currently playing track
int spotify_get_track_info(int *channels, int *rate) {
	spfs_audio *audio = NULL;
	int duration = 0;
	g_mutex_lock(&(g_playback->mutex));
	if (!g_playback->playing) {
		g_mutex_unlock(&g_playback->mutex);
		return 0;
	}

	while ((audio = g_queue_peek_head(g_playback->queue)) == NULL ) {
		g_cond_wait(&(g_playback->cond), &(g_playback->mutex));
	}
	if (channels)
		*channels = audio->channels;

	if (rate)
		*rate = audio->rate;

	if (wait_on_track(g_playback->playing))
		duration = sp_track_duration(g_playback->playing);

	g_mutex_unlock(&(g_playback->mutex));
	return duration;
}

GArray *spotify_get_playlists(sp_session *session) {
	g_mutex_lock(&g_spotify_api_mutex);
	sp_playlistcontainer *plc = sp_session_playlistcontainer(session);
	if (!wait_on_playlistcontainer(plc)) {
		g_warn_if_reached();
		return NULL;
	}
	int n = sp_playlistcontainer_num_playlists(plc);
	GArray *playlists = g_array_sized_new(false, false, sizeof(sp_playlist *), n);
	for (int i = 0; i < n; ++i) {
		sp_playlist * pl = sp_playlistcontainer_playlist(plc, i);
		g_array_insert_val(playlists, i, pl);
	}
	g_mutex_unlock(&g_spotify_api_mutex);
	return playlists;
}

GArray *spotify_get_playlist_tracks(sp_playlist *playlist) {
	g_mutex_lock(&g_spotify_api_mutex);
	if (!wait_on_playlist(playlist)) {
		g_warn_if_reached();
		return NULL;
	}
	int n = sp_playlist_num_tracks(playlist);
	GArray *tracks = g_array_sized_new(false, false, sizeof(sp_track *), n);
	for (int i = 0; i < n; ++i) {
		sp_track *track = sp_playlist_track(playlist, i);
		g_array_insert_val(tracks, i, track);
	}
	g_mutex_unlock(&g_spotify_api_mutex);
	return tracks;
}

// Tries to saturate buf with size bytes. Makes an effort to always return at least
// some data when a track is playing, even if this means waiting for it to be
// delivered.
size_t spotify_get_audio(char *buf, size_t size) {
	size_t sz = 0;
	spfs_audio *audio = NULL;

	if (!spotify_is_playing()) {
		g_debug("spotify not playing, no audio to get");
		return 0;
	}

	if (g_playback_done) {
		g_debug("spotify not playing, no audio to get");
		g_playback_done = false;
		return 0;
	}

	g_mutex_lock(&(g_playback->mutex));
	while (g_queue_is_empty(g_playback->queue)) {
		g_playback->stutter += 1;
		g_cond_wait(&(g_playback->cond), &(g_playback->mutex));
	}
	while ((audio = g_queue_pop_head(g_playback->queue)) != NULL) {
		size_t seg_sz = audio->nsamples * sizeof(int16_t) * audio->channels;
		if (sz + seg_sz <= size) { // the entire segment fits
			memcpy(buf+sz, audio->samples, seg_sz);
			g_playback->nsamples -= audio->nsamples;
			spfs_audio_free(audio);
			sz += seg_sz;
		}
		else if (sz == 0) { // we have to return something, at least
			size_t read_samples = size / (sizeof(int16_t) * audio->channels);

			// yield the first <size> bytes
			memcpy(buf, audio->samples, size);

			// and move the remaining samples to the beginning
			memmove(audio->samples, (char *)audio->samples + size, seg_sz - size);
			audio->nsamples -= read_samples;
			g_playback->nsamples -= read_samples;

			// re-add the partial segment
			g_queue_push_head(g_playback->queue, audio);
			sz = size;
			break;
		}
		else {
			g_queue_push_head(g_playback->queue, audio);
			break;
		}
	}
	g_mutex_unlock(&(g_playback->mutex));
	return sz;
}


void spotify_seek_track(sp_session *session, int offset) {
	g_return_if_fail(session != NULL);
	g_return_if_fail((int)offset >= 0);
	if (!g_playback->playing)
		g_warn_if_reached();

	g_playback_done = false;
	g_mutex_lock(&g_spotify_api_mutex);
	sp_session_player_seek(session, offset);
	g_mutex_unlock(&g_spotify_api_mutex);
	spfs_audio_playback_flush(g_playback);
}

bool spotify_play_track(sp_session *session, sp_track *track) {
	g_return_val_if_fail(session != NULL, false);
	g_return_val_if_fail(track != NULL, false);
	bool ret = true;
	if (g_playback->playing && g_playback->playing == track)
		return true;

	spfs_audio_playback_flush(g_playback);
	g_playback_done = false;
	g_mutex_lock(&g_spotify_api_mutex);
	sp_error sperr = sp_session_player_load(session, track);
	if(sperr == SP_ERROR_OK) {
		sp_session_player_play(session, true);
	}
	else {
		g_warning("Error loading track: %s", sp_error_message(sperr));
		ret = false;
	}
	g_mutex_unlock(&g_spotify_api_mutex);
	g_playback->playing = track;
	return ret;
}

GSList *spotify_artist_search(sp_session * session, const char *query) {
	int i = 0, num_artists = 0;
	sp_search *search = NULL;
	sp_artist *artist = NULL;
	GSList *artists = NULL;
	g_return_val_if_fail(session != NULL, NULL);
	g_return_val_if_fail(query != NULL, NULL);

	sp_connectionstate connstate = spotify_connectionstate(session);
	if ( connstate != SP_CONNECTION_STATE_LOGGED_IN) {
		g_debug("Not logged in (%s), won't search for artist %s", spotify_connectionstate_str(connstate), query);
		return NULL;
	}
	g_mutex_lock(&g_spotify_api_mutex);
	search = sp_search_create(session, query, 0, 0, 0, 0, 0, 100, 0, 0, SP_SEARCH_STANDARD, artist_search_complete_cb, NULL);
	g_debug("search created, waiting on load");

	gint64 end_time = g_get_monotonic_time() + SPFS_CB_TIMEOUT_S * G_TIME_SPAN_SECOND;
	while (!sp_search_is_loaded(search)) {
		if (!g_cond_wait_until(&g_spotify_data_available, &g_spotify_api_mutex, end_time))
		{
			g_mutex_unlock(&g_spotify_api_mutex);
			g_debug("still not loaded...giving up");
			return NULL;
		}
	}

	num_artists = sp_search_num_artists(search);
	g_debug("Found %d artists", num_artists);
	if (num_artists > 0) {
		for (i = 0; i < num_artists; i++) {
			artist = sp_search_artist(search, i);
			artists = g_slist_append(artists, artist); /*FIXME: sp_artist_ref?*/
			g_debug("Found artist: %s", sp_artist_name(artist));
		}
	}
	sp_search_release(search);
	g_mutex_unlock(&g_spotify_api_mutex);
	return artists;
}

/* Playlist "getters"*/
SPFS_SPOTIFY_API_FUNC(const gchar *, playlist, name)
SPFS_SPOTIFY_API_FUNC(int, playlist, num_tracks)
SPFS_SPOTIFY_API_FUNC2(sp_track *, playlist, track, int, index)
SPFS_SPOTIFY_API_FUNC2(int, playlist, track_create_time, int, index)

/* Artist "getters"*/
SPFS_SPOTIFY_API_FUNC(const gchar *, artist, name)

/* Track "getters"*/
SPFS_SPOTIFY_API_FUNC(const gchar *, track, name)
SPFS_SPOTIFY_API_FUNC(int, track, duration)
SPFS_SPOTIFY_API_FUNC(int, track, disc)
SPFS_SPOTIFY_API_FUNC(int, track, index)
SPFS_SPOTIFY_API_FUNC(int, track, popularity)
SPFS_SPOTIFY_API_FUNC(int, track, num_artists)
SPFS_SPOTIFY_API_FUNC(sp_track_offline_status, track, offline_get_status)
SPFS_SPOTIFY_API_FUNC2(sp_artist *, track, artist, int, index)
SPFS_SPOTIFY_SESSION_API_FUNC(bool, track, is_starred)
SPFS_SPOTIFY_SESSION_API_FUNC(bool, track, is_local)
SPFS_SPOTIFY_SESSION_API_FUNC(bool, track, is_autolinked)

/* Playlist container "getters"*/
SPFS_SPOTIFY_API_FUNC(int, playlistcontainer, num_playlists)
SPFS_SPOTIFY_API_FUNC2(sp_playlist *, playlistcontainer, playlist, int, index)

/* Session "getters" */
SPFS_SPOTIFY_API_FUNC(sp_playlistcontainer *, session, playlistcontainer)

sp_link * spotify_link_create_from_artist(sp_artist *artist) {
	sp_link *link = NULL;
	g_mutex_lock(&g_spotify_api_mutex);
	link = sp_link_create_from_artist(artist);
	g_mutex_unlock(&g_spotify_api_mutex);
	return link;
}

sp_link * spotify_link_create_from_track(sp_track *track) {
	sp_link *link = NULL;
	g_mutex_lock(&g_spotify_api_mutex);
	link = sp_link_create_from_track(track,
			0 // offset in ms
			);
	g_mutex_unlock(&g_spotify_api_mutex);
	return link;
}

sp_link * spotify_link_create_from_string(const char *str) {
	sp_link *link = NULL;
	g_mutex_lock(&g_spotify_api_mutex);
	link = sp_link_create_from_string(str);
	g_mutex_unlock(&g_spotify_api_mutex);
	return link;
}

sp_artist * spotify_link_as_artist(sp_link *link) {
	sp_artist *artist= NULL;
	g_mutex_lock(&g_spotify_api_mutex);
	artist = sp_link_as_artist(link);
	g_mutex_unlock(&g_spotify_api_mutex);
	return artist;
}


int spotify_link_as_string(sp_link *link, char *buf, int buffer_size) {
	int ret = 0;
	g_mutex_lock(&g_spotify_api_mutex);
	ret = sp_link_as_string(link, buf, buffer_size);
	if (ret >= buffer_size) {
		g_warning("Link truncated!");
	}
	g_mutex_unlock(&g_spotify_api_mutex);
	return ret;

}

/*thread routine*/
void * spotify_thread_start(void *arg) {
	int event_timeout = 0;
	sp_error err;
	sp_session * session = (sp_session *) arg;
	g_return_val_if_fail(session != NULL, NULL);
	g_debug("spotify session processing thread: started");
	g_mutex_lock(&g_spotify_notify_mutex);
	while (g_running) {
		if (event_timeout == 0) {
			while (!g_main_thread_do_notify) {
				g_debug("spotify session processing thread: waiting on notify mutex");
				g_cond_wait(&g_spotify_notify_cond, &g_spotify_notify_mutex);
			}
		}
		else {
			gint64 end_time = g_get_monotonic_time () + event_timeout;
			g_cond_wait_until(&g_spotify_notify_cond, &g_spotify_notify_mutex, end_time);
		}
		g_main_thread_do_notify = false;

		g_mutex_unlock(&g_spotify_notify_mutex);

		do {
			g_mutex_lock(&g_spotify_api_mutex);
			err = sp_session_process_events(session, &event_timeout);
			g_mutex_unlock(&g_spotify_api_mutex);
			if (err != SP_ERROR_OK) {
				g_warning("spotify session processing thread: Could not process events (%d): %s\n", err, sp_error_message(err));
			}
		} while (event_timeout == 0);
		g_mutex_lock(&g_spotify_notify_mutex);
	}
	g_debug("spotify session processing thread: ended");
	return (void *)NULL;
}

/* "public" convenience functions */
const char * spotify_connectionstate_str(sp_connectionstate connectionstate) {
	switch (connectionstate) {
		case SP_CONNECTION_STATE_LOGGED_OUT:
			return "logged out";
			break;
		case SP_CONNECTION_STATE_LOGGED_IN:
			return "logged in";
			break;
		case SP_CONNECTION_STATE_DISCONNECTED:
			return "disconnected";
			break;
		case SP_CONNECTION_STATE_OFFLINE:
			return "offline";
			break;
		case SP_CONNECTION_STATE_UNDEFINED:
			return "undefined";
			break;
	}
	return "undefined";
}


const char * spotify_track_offline_status_str(sp_track_offline_status offlinestatus) {
	switch (offlinestatus) {
		case SP_TRACK_OFFLINE_NO:
			return "Not marked for offline"; break;
		case SP_TRACK_OFFLINE_WAITING:
			return "Waiting for download"; break;
		case SP_TRACK_OFFLINE_DOWNLOADING:
			return "Currently downloading."; break;
		case SP_TRACK_OFFLINE_DONE:
			return "Downloaded OK and can be played."; break;
		case SP_TRACK_OFFLINE_ERROR:
			return "Error during download."; break;
		case SP_TRACK_OFFLINE_DONE_EXPIRED:
			return "Downloaded OK but not playable due to expiry."; break;
		case SP_TRACK_OFFLINE_LIMIT_EXCEEDED:
			return "Waiting because device have reached max number of allowed tracks."; break;
		case SP_TRACK_OFFLINE_DONE_RESYNC:
			return "Downloaded OK and available but scheduled for re-download."; break;
	}
	return "undefined";
}
