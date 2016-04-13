#ifndef SPFS_FUSE_TRACK_H
#define SPFS_FUSE_TRACK_H
#include <libspotify/api.h>
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
spfs_entity *create_track_wav_file(const gchar *name, sp_track *track);
spfs_entity *create_track_browse_dir(sp_track *track);
void create_wav_track_link_in_directory(spfs_entity *dir, const gchar *linkname, sp_track *track, struct stat_times *times);
#endif /* SPFS_FUSE_TRACK_H */
