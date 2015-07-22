#include "spfs_fuse_entity.h"
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

	child->parent = parent;
	g_hash_table_insert(parent->e.dir->children, child->name, child);
}

void spfs_entity_link_set_target(spfs_entity *link, gchar *target) {
	g_return_if_fail(link != NULL);
	g_return_if_fail(target != NULL);
	g_return_if_fail(link->type == S_IFLNK);

	link->e.link->target = target;
}

static gchar * sanitize_name(const gchar *n) {
	gchar *san_name = g_strdup(n), *p = san_name;

	while (*p != '\0') {
		switch (*p) {
			case '/': *p = ' '; break;
			default: break;
		}
		++p;
	}
	return san_name;
}

bool spfs_entity_dir_has_child(spfs_dir *dir, const char *name) {
	bool ret = false;
	g_return_val_if_fail(dir != NULL, false);
	gchar *san_name = sanitize_name(name);
	ret = g_hash_table_lookup(dir->children, san_name) != NULL;
	g_free(san_name);
	return ret;
}

static spfs_entity * spfs_entity_create(const gchar *name, SpfsEntityType type) {
	spfs_entity *e = g_new0(spfs_entity, 1);
	e->type = type;
	if (name != NULL) {
		e->name = sanitize_name(name);
	}
	e->auxdata = NULL;
	time_t t = time(NULL);
	e->atime = t;
	e->ctime = t;
	e->mtime = t;
	return e;
}

static void spfs_file_destroy(spfs_file *file) {
	g_return_if_fail(file != NULL);
	g_free(file);
}

static void spfs_link_destroy(spfs_link *link) {
	g_return_if_fail(link != NULL);
	g_free(link);
}

static void spfs_dir_destroy(spfs_dir *dir) {
	g_return_if_fail(dir != NULL);
	g_hash_table_destroy(dir->children);
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

spfs_entity * spfs_entity_file_create(const gchar *name, SpfsReadFunc read_func) {
	g_return_val_if_fail(name != NULL, NULL);
	spfs_entity *e = spfs_entity_create(name, SPFS_FILE);
	e->e.file = g_new0(spfs_file, 1);
	e->e.file->read = read_func;
	g_debug("created file %s", name);
	return e;
}

spfs_entity * spfs_entity_dir_create(const gchar *name, SpfsReaddirFunc readdir_func) {
	spfs_entity *e = spfs_entity_create(name, SPFS_DIR);
	e->parent = NULL;
	e->e.dir = g_new0(spfs_dir, 1);
	e->e.dir->readdir = readdir_func;
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

spfs_entity *spfs_entity_root_create(SpfsReaddirFunc readdir_func) {
	return spfs_entity_dir_create(NULL, readdir_func);
}

spfs_entity *spfs_entity_link_create(const gchar *name, SpfsReadlinkFunc readlink_func) {
	g_return_val_if_fail(name != NULL, NULL);
	spfs_entity *e = spfs_entity_create(name, SPFS_LINK);
	e->e.link = g_new(spfs_link, 1);
	e->e.link->readlink = readlink_func;
	g_debug("created link %s", name);
	return e;
}

