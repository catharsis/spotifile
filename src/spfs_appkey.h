#ifndef SPFS_APPKEY_H
#define SPFS_APPKEY_H
#include <stdlib.h>
/**
 * Returns the libspotify appkey.
 * @param[out] ksz The size of the appkey.
 */
void * spfs_appkey_get(size_t *ksz);
#endif /* SPFS_APPKEY_H */
