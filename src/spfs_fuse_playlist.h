#ifndef SPFS_FUSE_PLAYLIST_H
#define SPFS_FUSE_PLAYLIST_H
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
int playlists_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
#endif /* SPFS_FUSE_PLAYLIST_H */
