#ifndef SPFS_FUSE_ARTIST_H
#define SPFS_FUSE_ARTIST_H
#include <libspotify/api.h>
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
spfs_entity *create_artist_browse_dir(sp_artist *artist);
#endif /* SPFS_FUSE_ARTIST_H */
