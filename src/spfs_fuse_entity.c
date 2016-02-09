#include "spfs_fuse_entity.h"
#include "spfs_fuse_utils.h"
#include "spfs_path.h"
#include <string.h>
#include <unistd.h>

spfs_entity * spfs_entity_find_path(spfs_entity *root, const gchar *path) {
	g_return_val_if_fail(root != NULL, NULL);
	g_return_val_if_fail(root->name == NULL, NULL);
	g_return_val_if_fail(path != NULL, NULL);

	if (strlen(path) <= 0) {
		return NULL;
	}
	if (g_strcmp0(path, "/") == 0) {
		return root;
	}

	gchar **elems = g_strsplit(path, "/", 0);
	gchar *cur_el = NULL;
	spfs_entity *found = root;

	size_t i = 0;
	while ((elems[i] != NULL) && found != NULL) {
		/*Skip leading empty elements*/
		while (elems[i+1] != NULL && strlen(elems[i]) == 0) {
			++i;
			continue;
		}
		cur_el = elems[i];

		/*Skip trailing empty elements*/
		while (elems[i+1] != NULL && strlen(elems[i+1]) == 0) {
			++i;
		}

		if (elems[i+1] == NULL && (g_strcmp0(cur_el, found->name) == 0)) {
			break;
		}
		else if (found->type == SPFS_DIR) {
			GHashTable *children = found->e.dir->children;
			found = g_hash_table_lookup(children, cur_el);
		}
		else {
			/* Where do we go now, but nowhere? */
			found = NULL;
		}
		++i;
	}
	g_strfreev(elems);
	return found;
}

gchar *spfs_entity_get_full_path(spfs_entity *e) {
	g_return_val_if_fail(e != NULL, NULL);
	if (e->name == NULL) {
		return g_strdup("/");
	}

	gchar *p = g_malloc0(PATH_MAX);
	GArray *elems = g_array_new(FALSE, FALSE, sizeof(gpointer));

	while (e != NULL) {
		if (e->name != NULL) {
			gchar *s = g_strdup(e->name);
			g_array_append_val(elems, s);
		}
		e = e->parent;
	}

	size_t i;
	for (i = elems->len; i > 0; i--) {
		gchar *el = g_array_index(elems, gchar*, i-1);
		g_strlcat(p, "/", PATH_MAX);
		g_strlcat(p, el, PATH_MAX);
		g_free(el);
	}
	g_array_free(elems, TRUE);
	return p;
}

gchar *relpath(spfs_entity *from, spfs_entity *to) {
	gchar *frompath = spfs_entity_get_full_path(from);
	gchar *topath = spfs_entity_get_full_path(to);

	gchar * relpath = spfs_path_get_relative_path(frompath, topath);
	if (!relpath)
		g_warn_if_reached();

	g_free(frompath);
	g_free(topath);
	return relpath;
}

unsigned int spfs_entity_get_direct_io(spfs_entity *e) {
	g_return_val_if_fail(e != NULL, 0);
	if (e->type != SPFS_FILE || e->e.file->size > 0)
		return 0;

	return 1;

}

void spfs_entity_stat(spfs_entity *e, struct stat *statbuf) {
	switch (e->type) {
		case SPFS_DIR:
			statbuf->st_mode = S_IFDIR | S_IRUSR | S_IXUSR;
			break;
		case SPFS_FILE:
			statbuf->st_mode = S_IFREG | S_IRUSR | S_IRGRP;
			statbuf->st_size = e->e.file->size;
			break;
		case SPFS_LINK:
			statbuf->st_mode = S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;
			if (e->e.link->target)
				statbuf->st_size = strlen(e->e.link->target);
			break;
	}

	statbuf->st_uid = getuid();
	if (e->ctime)
		statbuf->st_ctime = e->ctime;
	if (e->mtime)
		statbuf->st_mtime = e->mtime;
	if (e->atime)
		statbuf->st_atime = e->atime;

}
void spfs_entity_dir_add_child(spfs_entity *parent, spfs_entity *child) {
	g_return_if_fail(parent != NULL);
	g_return_if_fail(child != NULL);
	g_return_if_fail(parent->type == S_IFDIR);
	g_return_if_fail(strlen(child->name) > 0);

	if (spfs_entity_dir_has_child(parent->e.dir, child->name)) {
		g_warning("cowardly refusing to overwrite existing child %s in directory %s", child->name, parent->name);
		return;
	}
	child->parent = parent;
	g_hash_table_insert(parent->e.dir->children, child->name, child);
	g_debug("added %s to dir %s", child->name, parent->name ? parent->name : "root");
}

void spfs_entity_link_set_target(spfs_entity *link, const gchar *target) {
	g_return_if_fail(link != NULL);
	g_return_if_fail(target != NULL);
	g_return_if_fail(link->type == S_IFLNK);

	g_free(link->e.link->target);
	link->e.link->target = g_strdup(target);
}

spfs_entity * spfs_entity_dir_get_child(spfs_dir *dir, const char *name) {
	g_return_val_if_fail(dir != NULL, false);
	gchar *san_name = spfs_sanitize_name(name);
	spfs_entity *e = g_hash_table_lookup(dir->children, san_name);
	g_free(san_name);
	return e;
}

bool spfs_entity_dir_has_child(spfs_dir *dir, const char *name) {
	return spfs_entity_dir_get_child(dir, name) != NULL;
}

static spfs_entity * spfs_entity_create(const gchar *name, SpfsEntityType type) {
	spfs_entity *e = g_new0(spfs_entity, 1);
	e->type = type;
	if (name != NULL) {
		e->name = spfs_sanitize_name(name);
	}
	e->auxdata = NULL;
	e->refs = 0;
	time_t t = time(NULL);
	e->atime = t;
	e->ctime = t;
	e->mtime = t;
	return e;
}

static void spfs_file_destroy(spfs_file *file) {
	g_return_if_fail(file != NULL);
	g_free(file->ops);
	g_free(file);
}

static void spfs_link_destroy(spfs_link *link) {
	g_return_if_fail(link != NULL);
	g_free(link->ops);
	g_free(link);
}

static void spfs_dir_destroy(spfs_dir *dir) {
	g_return_if_fail(dir != NULL);
	g_hash_table_destroy(dir->children);
	g_free(dir->ops);
	g_free(dir);
}

void spfs_entity_destroy(spfs_entity *e) {
	g_return_if_fail(e != NULL);
	switch (e->type) {
	case SPFS_DIR:
		spfs_dir_destroy(e->e.dir);
		break;
	case SPFS_LINK:
		spfs_link_destroy(e->e.link);
		break;
	case SPFS_FILE:
		spfs_file_destroy(e->e.file);
		break;
	}
	g_free(e->name);
	g_free(e);
}

static void spfs_entity_ht_destroy(gpointer p) {
	spfs_entity *e = (spfs_entity *)p;
	spfs_entity_destroy(e);
}

static gboolean spfs_entity_ht_key_compare(gconstpointer a, gconstpointer b) {
	return (g_strcmp0(a, b) == 0);
}

spfs_entity * spfs_entity_file_create(const gchar *name, struct spfs_file_ops *ops) {
	g_return_val_if_fail(name != NULL, NULL);
	spfs_entity *e = spfs_entity_create(name, SPFS_FILE);
	e->e.file = g_new0(spfs_file, 1);
	e->e.file->ops = g_new0(struct spfs_file_ops, 1);
	if (ops != NULL) {
		e->e.file->ops->read = ops->read;
		e->e.file->ops->open = ops->open;
		e->e.file->ops->release = ops->release;
	}

	g_debug("created file %s", name);
	return e;
}

spfs_entity * spfs_entity_dir_create(const gchar *name, struct spfs_dir_ops *ops) {
	spfs_entity *e = spfs_entity_create(name, SPFS_DIR);
	e->parent = NULL;
	e->e.dir = g_new0(spfs_dir, 1);
	e->e.dir->ops = g_new0(struct spfs_dir_ops, 1);
	if (ops != NULL) {
		e->e.dir->ops->readdir = ops->readdir;
		e->e.dir->ops->mkdir = ops->mkdir;
	}

	e->e.dir->children = g_hash_table_new_full(g_str_hash,
			spfs_entity_ht_key_compare,
			NULL,
			spfs_entity_ht_destroy);
	if (name)
		g_debug("created dir %s", name);
	else
		g_debug("created root");
	return e;
}

spfs_entity *spfs_entity_root_create(struct spfs_dir_ops * ops) {
	return spfs_entity_dir_create(NULL, ops);
}

spfs_entity *spfs_entity_link_create(const gchar *name, struct spfs_link_ops *ops) {
	g_return_val_if_fail(name != NULL, NULL);
	spfs_entity *e = spfs_entity_create(name, SPFS_LINK);
	e->e.link = g_new0(spfs_link, 1);
	e->e.link->ops = g_new0(struct spfs_link_ops, 1);
	if (ops != NULL) {
		e->e.link->ops->readlink = ops->readlink;
	}
	e->e.link->target = g_strdup(e->name);
	g_debug("created link %s", name);
	return e;
}

