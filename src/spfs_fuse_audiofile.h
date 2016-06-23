#ifndef SPFS_FUSE_AUDIOFILE_H
#define SPFS_FUSE_AUDIOFILE_H
#include "spfs_fuse.h"
/**
 * open() implementation for a wav audio file
 */
int wav_open(struct fuse_file_info *fi);

/**
 * read() implementation for a wav audio file
 */
int wav_read(struct fuse_file_info *fi, char *buf, size_t size, off_t offset);

/**
 * release() implementation for a wav audio file
 */
int wav_release(struct fuse_file_info *fi);
#endif /* SPFS_FUSE_AUDIOFILE_H */
