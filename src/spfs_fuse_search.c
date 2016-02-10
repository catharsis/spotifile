#include "spfs_fuse_search.h"
#include <glib.h>
static int search_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	return 0;
}

int search_dir_mkdir(spfs_entity *parent, const char *dirname, mode_t mode) {
	gchar * parent_full_path = spfs_entity_get_full_path(parent);
	g_debug("creating search directory %s in %s", dirname, parent_full_path);
	g_free(parent_full_path);
	/*TODO: create the sp_search, but don't wait for it to complete, that should
	 * be done in search_readdir - Keep It Lazy
	 */
	spfs_entity_dir_add_child(parent,
			spfs_entity_dir_create(dirname,
				&(struct spfs_dir_ops){
				.mkdir = NULL,
				.readdir = search_readdir
				})
			);
	return 0;
}
