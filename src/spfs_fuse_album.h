#ifndef SPFS_FUSE_ALBUM_H
#define SPFS_FUSE_ALBUM_H
#include <libspotify/api.h>
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
/**
 * Creates a browse directory in browse/albums/ for the supplied \p sp_album
 * @param[in] album The album to create a directory for
 * @return The new directory entity
 */
spfs_entity *create_album_browse_dir(sp_album *album);
#endif /* SPFS_FUSE_ALBUM_H */
