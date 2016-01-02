#ifndef SPFS_FUSE_ARTIST_H
#define SPFS_FUSE_ARTIST_H
#include <libspotify/api.h>
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
/**
 * Creates a new browse directory in browse/artists for the supplied
 * \p sp_artist
 * @param[in] artist The artist to create a directory for
 * @return The new directory entity
 */
spfs_entity *create_artist_browse_dir(sp_artist *artist);
#endif /* SPFS_FUSE_ARTIST_H */
