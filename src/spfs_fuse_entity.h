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
typedef int (*SpfsMkdirFunc)(const char *path, mode_t mode);


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
	SpfsMkdirFunc mkdir;
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


/**
 * Returns the full path from the root of the filesystem
 * to entity \p e
 * @param[in] e The \p spfs_entity to return the full path to
 * @return The full path. This should be freed by the user.
 */
gchar *spfs_entity_get_full_path(spfs_entity *e);

/**
 * Returns the relative path from entity \p from to entity \p to
 * @param[in] from The origin entity
 * @param[in] to The target entity
 * @return The relative path. This should be freed by the user.
 */
gchar *relpath(spfs_entity *from, spfs_entity *to);

/**
 * Given a root entity \p root and a path \p path, returns the entity
 * located at \p path under \p root, if it exists.
 * @param[in] root The search root
 * @param[in] path The path to find
 * @return The entity at \p path, if one exists. Otherwise null.
 */
spfs_entity * spfs_entity_find_path(spfs_entity *root, const gchar *path);

/**
 * Determines, based on super-advanced heuristics, whether a given entity
 * should have direct I/O enabled or not. The determining factors are as
 * follows:
 *  - If \p e is a file and \p e has an (internal) size of zero or less,
 *    direct I/O is enabled.
 *  - Otherwise, it's not.
 *
 * @param[in] e The entity
 * @return 1 if direct I/O is recommended, 0 otherwise.
 */
unsigned int spfs_entity_get_direct_io(spfs_entity *e);

/**
 * Fills the stat struct \p statbuf according to the information in the
 * supplied \p spfs_entity
 *
 * @param[in] e The entity
 * @param[out] statbuf The stat struct
 */
void spfs_entity_stat(spfs_entity *e, struct stat *statbuf);

/**
 * Frees the entity \p e and its associated data. If \p e is a directory, all of
 * its children are destroyed as well.
 * @param[in] e The entity
 */
void spfs_entity_destroy(spfs_entity *e);

/**
 * Creates and returns a new root directory entity
 * @param[in] ops The directory operations for the root
 * @return The new root directory entity. This should be passed to
 * \p spfs_entity_destroy() when it's no longer needed
 */
spfs_entity * spfs_entity_root_create(struct spfs_dir_ops *ops);

/**
 * Creates and returns a new file entity
 * @param[in] name The name of the file
 * @param[in] ops The file operations for this file
 * @return The new file entity. This should be passed to
 * \p spfs_entity_destroy() when it's no longer needed
 */
spfs_entity * spfs_entity_file_create(const gchar *name, struct spfs_file_ops *ops);

/**
 * Creates and returns a new directory entiry
 * @param[in] name The name of the directory
 * @param[in] ops The directory operations for this directory
 * @return The new directory entity. This should be passed to
 * \p spfs_entity_destroy() when it's no longer needed
 */
spfs_entity * spfs_entity_dir_create(const gchar *name, struct spfs_dir_ops *ops);

/**
 * Adds a child entity to a directory. After calling this function, \p child
 * should NOT be freed by the user. It is now owned by the parent directory.
 *
 * @param[in] parent The parent directory
 * @param[in] child The child directory
 */
void spfs_entity_dir_add_child(spfs_entity *parent, spfs_entity *child);

/**
 * Finds and returns the child with name \p name in the directory \p dir. If
 * no child exists with the given name, NULL is returned.
 *
 * @param[in] dir The directory
 * @param[in] name The child name
 * @return The found entity, or NULL if it was not found
 */
spfs_entity * spfs_entity_dir_get_child(spfs_dir *dir, const char *name);

/**
 * Checks if the directory has a child with name \p name.
 * @param[in] dir The directory
 * @param[in] name The child name
 * @return TRUE if the directory has such a child, FALSE otherwise
 */
bool spfs_entity_dir_has_child(spfs_dir *dir, const char *name);

/**
 * Creates and returns a new link entity
 * @param[in] name The name of the link
 * @param[in] ops The link operations for this link
 * @return The new link entity. This should be passed to
 * \p spfs_entity_destroy() when it's no longer needed
 */
spfs_entity *spfs_entity_link_create(const gchar *name, struct spfs_link_ops *ops);

/**
 * Sets the target path for the link entity \p link
 * @param[in] link The link
 * @param[in] target The target path
 */
void spfs_entity_link_set_target(spfs_entity *link, const gchar *target);

#endif /* SPFS_FUSE_ENTITY_H */
