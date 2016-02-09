#include "spfs_fuse_search.h"
#include <glib.h>
int search_dir_mkdir(const char *path, mode_t mode) {
	g_debug("creating search directory %s", path);
	return 0;
}
