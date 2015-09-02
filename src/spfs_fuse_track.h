#ifndef SPFS_FUSE_TRACK_H
#define SPFS_FUSE_TRACK_H
#include <libspotify/api.h>
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
spfs_entity *create_track_wav_file(const gchar *name, sp_track *track);
spfs_entity *create_track_browse_dir(sp_track *track);
#endif /* SPFS_FUSE_TRACK_H */
