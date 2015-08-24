#ifndef SPFS_FUSE_AUDIOFILE_H
#define SPFS_FUSE_AUDIOFILE_H
#include "spfs_fuse.h"
int wav_open(const char *path, struct fuse_file_info *fi);
int wav_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int wav_release(const char *path, struct fuse_file_info *fi);
#endif /* SPFS_FUSE_AUDIOFILE_H */
