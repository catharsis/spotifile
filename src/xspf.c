#include <xspf.h>
#include "xspf_sanitize.h"

enum XSPFState {
	INIT,
	PLAYLIST_BEGIN,
	TRACKLIST_BEGIN,
	TRACK_BEGIN
};

struct xspf {
	enum XSPFState state;
	GString *string;
};

xspf * xspf_new(void) {
	xspf *x = g_new0(xspf, 1);
	x->string = g_string_new("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	x->state = INIT;
	return x;
}

char * xspf_free(xspf *x, gboolean free_segment) {
	char * ret = g_string_free(x->string, free_segment);
	g_free(x);
	return ret;
}

xspf * xspf_begin_playlist(xspf *x) {
	g_warn_if_fail(x->state == INIT);
	g_string_append(x->string, "<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n");
	x->state = PLAYLIST_BEGIN;
	return x;
}

xspf * xspf_end_playlist(xspf *x) {
	g_warn_if_fail(x->state == PLAYLIST_BEGIN);
	g_string_append(x->string, "</playlist>\n");
	x->state = INIT;
	return x;
}

xspf * xspf_begin_tracklist(xspf *x) {
	g_warn_if_fail(x->state == PLAYLIST_BEGIN);
	g_string_append(x->string, "<trackList>\n");
	x->state = TRACKLIST_BEGIN;
	return x;
}

xspf * xspf_end_tracklist(xspf *x) {
	g_warn_if_fail(x->state == TRACKLIST_BEGIN);
	g_string_append(x->string, "</trackList>\n");
	x->state = PLAYLIST_BEGIN;
	return x;
}

xspf * xspf_begin_track(xspf *x) {
	g_warn_if_fail(x->state == TRACKLIST_BEGIN);
	g_string_append(x->string, "<track>\n");
	x->state = TRACK_BEGIN;
	return x;
}

xspf * xspf_end_track(xspf *x) {
	g_warn_if_fail(x->state == TRACK_BEGIN);
	g_string_append(x->string, "</track>\n");
	x->state = TRACKLIST_BEGIN;
	return x;
}

xspf * xspf_track_set_location(xspf *x, const gchar *location) {
	g_warn_if_fail(x->state == TRACK_BEGIN);
	/**
	 * http://www.xspf.org/xspf-v1.html:
	 *
	 * Generators should take extra care to ensure that relative paths are correctly encoded. Do:
	 *<location>My%20Song.flac</location>
	 *
	 *Don't:
	 *<location>My Song.flac</location>
	 */
	char *escaped_location = g_uri_escape_string(location, NULL, TRUE);
	g_string_append_printf(x->string, "<location>%s</location>\n", escaped_location);
	g_free(escaped_location);
	return x;
}

xspf * xspf_track_set_duration(xspf *x, int duration) {
	g_warn_if_fail(x->state == TRACK_BEGIN);
	g_string_append_printf(x->string, "<duration>%d</duration>\n", duration);
	return x;
}
xspf * xspf_track_set_title(xspf *x, const gchar *title) {
	g_warn_if_fail(x->state == TRACK_BEGIN);
	g_string_append_printf(x->string, "<title>%s</title>\n", xspf_escape_amperstand(title));
	return x;
}

xspf * xspf_track_set_creator(xspf *x, const gchar *creator) {
	g_warn_if_fail(x->state == TRACK_BEGIN);
	g_string_append_printf(x->string, "<creator>%s</creator>\n", creator);
	return x;
}

xspf * xspf_track_set_album(xspf *x, const gchar *album) {
	g_warn_if_fail(x->state == TRACK_BEGIN);
	g_string_append_printf(x->string, "<album>%s</album>\n", album);
	return x;
}
