#ifndef SPFS_AUDIO_H
#define SPFS_AUDIO_H
#include <glib.h>
#include <libspotify/api.h>
typedef struct spfs_audio {
	int channels;
	int rate;
	int nsamples;
	int16_t samples[0];
} spfs_audio;

typedef struct spfs_audio_playback {
	int nsamples;
	int stutter;
	GMutex mutex;
	GCond cond;
	GQueue *queue;
	sp_track *playing;
} spfs_audio_playback;

/**
 * Frees a \p spfs_audio
 * @param[in] audio
 */
void spfs_audio_free(spfs_audio *audio);


/**
 * Returns current playback status
 * @param[in] playback
 * @return true if playback is active, false otherwise
 */
bool spfs_audio_playback_is_playing(spfs_audio_playback *playback);
/**
 * Flushes (and frees) all playback data in the supplied
 * \p spfs_audio_playback
 * @param[in] playback
 */
void spfs_audio_playback_flush(spfs_audio_playback *playback);

/**
 * Allocates and returns a new \p spfs_audio_playback
 */
spfs_audio_playback *spfs_audio_playback_new(void);

/**
 * Frees the supplied \p spfs_audio_playback. For convenience, this also
 * flushes and frees all associated playback data.
 * @param[in] playback
 */
void spfs_audio_playback_free(spfs_audio_playback *playback);
#endif /* SPFS_AUDIO_H */
