#ifndef SPFS_FORMAT_H
#define SPFS_FORMAT_H
#include <glib.h>
#include <stdbool.h>
#include "spfs_spotify.h"
bool spfs_format_isvalid(const gchar *fmt);

GString * spfs_format_playlist_track_expand(const gchar *fmt, sp_track *track, int index);
#endif
