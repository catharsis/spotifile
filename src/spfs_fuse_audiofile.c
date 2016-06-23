#include "spfs_fuse_audiofile.h"
#include <libspotify/api.h>

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
	h->bits_per_sample = 16; //only supported sample type as of now
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

int wav_open(struct fuse_file_info *fi) {
	if ((SPFS_DATA)->music_playing)
		return -EBUSY;
	(SPFS_DATA)->music_playing = true;
	return 0;
}

int wav_release(struct fuse_file_info *fi) {
	if (G_UNLIKELY(!(SPFS_DATA)->music_playing))
		g_warn_if_reached();
	(SPFS_DATA)->music_playing = false;
	return 0;
}

int wav_read(struct fuse_file_info *fi, char *buf, size_t size, off_t offset) {
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

		size_t bytes_p_s = (channels * 16 * rate) / 8;

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
