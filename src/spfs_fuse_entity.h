#ifndef SPFS_FUSE_ENTITY_H
#define SPFS_FUSE_ENTITY_H

#include <glib.h>
#include <sys/stat.h>
#include <fuse.h>
#include <stdbool.h>
typedef int (*SpfsReadFunc)(const char *path, char *buff, size_t sz, off_t offset, struct fuse_file_info *fi);
typedef int (*SpfsOpenFunc)(const char *path, struct fuse_file_info *fi);
typedef int (*SpfsReleaseFunc)(const char *path, struct fuse_file_info *fi);

typedef int (*SpfsReaddirFunc)(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

typedef int (*SpfsReadlinkFunc)(const char *path, char *buf, size_t len);


struct spfs_entity;
typedef enum SpfsEntityType {
	SPFS_FILE = S_IFREG,
	SPFS_DIR = S_IFDIR,
	SPFS_LINK = S_IFLNK,
} SpfsEntityType;

struct spfs_file_ops {
	SpfsReadFunc read;
	SpfsOpenFunc open;
	SpfsReleaseFunc release;
};

struct spfs_dir_ops {
	SpfsReaddirFunc readdir;
	SpfsReleaseFunc release;
};

struct spfs_link_ops {
	SpfsReadlinkFunc readlink;
};

typedef struct spfs_file {
	struct spfs_file_ops *ops;
	size_t size;
	void *data;
} spfs_file;

typedef struct spfs_dir {
	struct spfs_dir_ops *ops;
	GHashTable *children;
} spfs_dir;

typedef struct spfs_link {
	struct spfs_link_ops *ops;
	gchar *target;
} spfs_link;


typedef struct spfs_entity {
	gchar *name;
	struct spfs_entity *parent;
	void *auxdata;
	SpfsEntityType type;
	unsigned int refs;
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
gchar *relpath(spfs_entity *from, spfs_entity *to);
spfs_entity * spfs_entity_find_path(spfs_entity *root, const gchar *path);
unsigned int spfs_entity_get_direct_io(spfs_entity *e);
void spfs_entity_stat(spfs_entity *e, struct stat *statbuf);
void spfs_entity_destroy(spfs_entity *e);

spfs_entity * spfs_entity_root_create(struct spfs_dir_ops *ops);

spfs_entity * spfs_entity_file_create(const gchar *name, struct spfs_file_ops *ops);

spfs_entity * spfs_entity_dir_create(const gchar *name, struct spfs_dir_ops *ops);
void spfs_entity_dir_add_child(spfs_entity *parent, spfs_entity *child);
spfs_entity * spfs_entity_dir_get_child(spfs_dir *dir, const char *name);
bool spfs_entity_dir_has_child(spfs_dir *dir, const char *name);

spfs_entity *spfs_entity_link_create(const gchar *name, struct spfs_link_ops *ops);
void spfs_entity_link_set_target(spfs_entity *link, const gchar *target);

#endif /* SPFS_FUSE_ENTITY_H */
