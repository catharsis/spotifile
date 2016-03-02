#ifndef SPFS_SEARCH_H
#define SPFS_SEARCH_H
#include <sys/types.h>
#include "spfs_fuse.h"
/**
 * mkdir() implementation for \p /search/
 */
int search_dir_mkdir(spfs_entity *parent, const char *dirname, mode_t mode);
#endif /* SPFS_SEARCH_H */
