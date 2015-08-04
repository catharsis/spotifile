#ifndef SPFS_FUSE_ALBUM_H
#define SPFS_FUSE_ALBUM_H
#include <libspotify/api.h>
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
spfs_entity *create_album_browse_dir(sp_album *album);
#endif /* SPFS_FUSE_ALBUM_H */
