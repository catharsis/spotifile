#include "spfs_audio.h"

void spfs_audio_free(spfs_audio *audio) {
	g_free(audio);
}

void spfs_audio_playback_flush(spfs_audio_playback *playback) {
	g_mutex_lock(&playback->mutex);
	spfs_audio *audio = NULL;
	while ((audio = g_queue_pop_head(playback->queue)) != NULL) {
		spfs_audio_free(audio);
	}
	playback->nsamples = 0;
	playback->stutter = 0;
	g_mutex_unlock(&playback->mutex);
}
spfs_audio_playback *spfs_audio_playback_new(void) {
	spfs_audio_playback *pbk = g_new0(spfs_audio_playback, 1);
	pbk->queue = g_queue_new();
	pbk->playing = NULL;
	g_cond_init(&pbk->cond);
	g_mutex_init(&pbk->mutex);
	return pbk;
}

void spfs_audio_playback_free(spfs_audio_playback *playback) {
	spfs_audio_playback_flush(playback);
	g_queue_free(playback->queue);
	g_cond_clear(&playback->cond);
	g_mutex_clear(&playback->mutex);
	g_free(playback);
}


