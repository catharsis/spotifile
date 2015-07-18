#ifndef SPFS_FUSE_ENTITY_H
#define SPFS_FUSE_ENTITY_H

#include <glib.h>
#include <sys/stat.h>
#include <fuse.h>
#include <stdbool.h>
typedef size_t (*SpfsReadFunc)(const char *path, char *buff, size_t sz, off_t offset, struct fuse_file_info *fi);

typedef int (*SpfsReaddirFunc)(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

typedef int (*SpfsReadlinkFunc)(const char *path, char *buf, size_t len);


struct spfs_entity;
typedef enum SpfsEntityType {
	SPFS_FILE = S_IFREG,
	SPFS_DIR = S_IFDIR,
	SPFS_LINK = S_IFLNK,
} SpfsEntityType;

typedef struct spfs_file {
	SpfsReadFunc read;
	size_t size;
	void *data;
} spfs_file;

typedef struct spfs_dir {
	GHashTable *children;
	SpfsReaddirFunc readdir;
} spfs_dir;

typedef struct spfs_link {
	gchar *target;
	SpfsReadlinkFunc readlink;
} spfs_link;


typedef struct spfs_entity {
	gchar *name;
	struct spfs_entity *parent;
	SpfsEntityType type;
	time_t ctime;
	time_t atime;
	time_t mtime;
	union {
		struct spfs_file *file;
		struct spfs_dir *dir;
		struct spfs_link *link;
	} e;
} spfs_entity;

gchar *spfs_entity_get_full_path(spfs_entity *e);
spfs_entity * spfs_entity_find_path(spfs_entity *root, const gchar *path);
void spfs_entity_stat(spfs_entity *e, struct stat *statbuf);
void spfs_entity_destroy(spfs_entity *e);

spfs_entity *spfs_entity_root_create(SpfsReaddirFunc readdir_func);

spfs_entity * spfs_entity_file_create(const gchar *name, SpfsReadFunc read_func);
spfs_entity * spfs_entity_dir_create(const gchar *name, SpfsReaddirFunc readdir_func);
void spfs_entity_dir_add_child(spfs_entity *parent, spfs_entity *child);
bool spfs_entity_dir_has_child(spfs_dir *dir, const char *name);

spfs_entity *spfs_entity_link_create(const gchar *name, SpfsReadlinkFunc readlink_func);
void spfs_entity_link_set_target(spfs_entity *link, gchar *target);

#endif /* SPFS_FUSE_ENTITY_H */
