#include "spfs_fuse_audiofile.h"
#include <libspotify/api.h>
#include <lame/lame.h>

#define BITS_PER_SAMPLE 16
struct wav_header {
	char riff[4];
	uint32_t size;
	char type[4];
	char fmt[4];
	uint32_t fmt_size;
	uint16_t fmt_type;
	uint16_t channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	char data[4];
	uint32_t data_size;
};

static void fill_wav_header(struct wav_header *h, int channels, int rate, int duration) {
	// A highly inflexible wave implementation, it gets the job done though
	// See, for example,
	// http://people.ece.cornell.edu/land/courses/ece4760/FinalProjects/f2014/wz233/ECE%204760%20Final%20Report%20%28HTML%29/ECE%204760%20Stereo%20Player_files/RIFF%20WAVE%20file%20format.jpg
	memcpy(h->riff, "RIFF", 4);
	h->bits_per_sample = BITS_PER_SAMPLE ; //only supported sample type as of now
	h->size = (sizeof(*h) + ((duration / 1000) * channels * h->bits_per_sample * rate) / 8) - 8;
	memcpy(h->type, "WAVE", 4);
	memcpy(h->fmt, "fmt ", 4);
	h->fmt_size = 16;
	h->fmt_type = 1; //PCM
	h->channels = channels;
	h->sample_rate = rate;
	h->byte_rate = (rate * h->bits_per_sample * channels) / 8;
	h->block_align = (h->bits_per_sample * channels) / 8;
	memcpy(h->data, "data", 4);
	h->data_size = h->size - 44;
}

int audiofile_open(const char *path, struct fuse_file_info *fi) {
	if ((SPFS_DATA)->music_playing)
		return -EBUSY;
	(SPFS_DATA)->music_playing = true;
	return 0;
}

int audiofile_release(const char *path, struct fuse_file_info *fi) {
	if (G_UNLIKELY(!(SPFS_DATA)->music_playing))
		g_warn_if_reached();
	(SPFS_DATA)->music_playing = false;
	return 0;
}

int wav_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	static off_t expoff = 0;
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	sp_session *session = SPFS_SP_SESSION;
	int channels, rate, duration;

	if (offset == 0) {
		if (!spotify_play_track(session, e->auxdata)) {
			g_warning("Failed to play track!");
			return -EINVAL;
		}
	}

	memset(buf, 0, size);
	struct wav_header header;
	size_t read = 0;


	if ((size_t) offset < sizeof(header)) {
		memset(&header, 0, sizeof(header));
		duration = spotify_get_track_info(&channels, &rate);
		if (duration < 0) {
			return 0;
		}


		fill_wav_header(&header, channels, rate, duration);

		if ( offset + size > sizeof(header))
			read = sizeof(header) - offset;
		else
			read = size;
		memcpy(buf, (char*)&header + offset, read);
	}

	if (expoff != offset) {
		//seek
		duration = spotify_get_track_info(&channels, &rate);
		if (duration < 0) {
			return 0;
		}

		size_t bytes_p_s = (channels * BITS_PER_SAMPLE * rate) / 8;

		int ms_offset = (offset / bytes_p_s) * 1000;

		if (duration < ms_offset)
			g_warn_if_reached();

		spotify_seek_track(session, ms_offset);
		expoff = offset;
	}

	if (read < size && (offset + read >= sizeof(header))) {
		// get some audio, but only if we've read the entire header
		read += spotify_get_audio(buf+read, size - read);
	}
	expoff += read;
	return read;
}

#ifdef HAVE_LIBLAME
int mp3_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	sp_session *session = SPFS_SP_SESSION;
	lame_global_flags *lgf = SPFS_DATA->lame_flags;
	int channels = 0, rate = 0;
	if (offset == 0) {
		if (!spotify_play_track(session, e->auxdata)) {
			g_warning("Failed to play track!");
			return -EINVAL;
		}
		(void) spotify_get_track_info(&channels, &rate);
		// set LAME params
		lame_set_num_channels(lgf, channels);
		lame_set_in_samplerate(lgf, rate);
		lame_set_VBR(lgf, vbr_default);
		g_return_val_if_fail(lame_init_params(lgf) >= 0, -EINVAL);
	}

	memset(buf, 0, size);
	size_t read = spotify_get_audio_mp3((char *)buf, size, lgf);
	return read;
}
#endif
