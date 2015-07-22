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

void spfs_audio_free(spfs_audio *audio);
void spfs_audio_playback_flush(spfs_audio_playback *playback);
spfs_audio_playback *spfs_audio_playback_new(void);
void spfs_audio_playback_free(spfs_audio_playback *playback);
#endif /* SPFS_AUDIO_H */
