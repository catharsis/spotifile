#ifndef XSPF_H
#define XSPF_H
#include <glib.h>

typedef struct xspf xspf;
xspf * xspf_new(void);
gchar * xspf_free(xspf *, gboolean free_segment);

xspf * xspf_begin_playlist(xspf *);
xspf * xspf_end_playlist(xspf *);

xspf * xspf_begin_tracklist(xspf *);
xspf * xspf_end_tracklist(xspf *);

xspf * xspf_begin_track(xspf *);
xspf * xspf_end_track(xspf *);

xspf * xspf_track_set_location(xspf *, const gchar *);
xspf * xspf_track_set_duration(xspf *, int);
xspf * xspf_track_set_title(xspf *x, const gchar *title);
xspf * xspf_track_set_creator(xspf *x, const gchar *creator);
xspf * xspf_track_set_album(xspf *x, const gchar *album);
#endif /* XSPF_H */
