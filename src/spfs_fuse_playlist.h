#ifndef SPFS_FUSE_PLAYLIST_H
#define SPFS_FUSE_PLAYLIST_H
#include "spfs_fuse.h"
#include "spfs_fuse_entity.h"
/**
 * readdir() implementation for \p /playlists/music/
 */
int playlists_music_dir_readdir(struct fuse_file_info *fi, void *buf, fuse_fill_dir_t filler, off_t offset);

/**
 * readdir() implementation for \p /playlists/meta/
 */
int playlists_meta_dir_readdir(struct fuse_file_info *fi, void *buf, fuse_fill_dir_t filler, off_t offset);
#endif /* SPFS_FUSE_PLAYLIST_H */
