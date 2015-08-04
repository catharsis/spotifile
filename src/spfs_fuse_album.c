#include "spfs_fuse_album.h"
#include "spfs_spotify.h"
static int name_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	gchar *str = g_strdup(spotify_album_name(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

spfs_entity *create_album_browse_dir(sp_album *album) {
	g_return_val_if_fail(album != NULL, NULL);
	spfs_entity *album_browse_dir = spfs_entity_find_path(SPFS_DATA->root, "/browse/albums");
	sp_link *link = spotify_link_create_from_album(album);

	g_return_val_if_fail(link != NULL, NULL);
	gchar album_linkstring[1024] = {0, };
	spotify_link_as_string(link, album_linkstring, 1024);
	if (spfs_entity_dir_has_child(album_browse_dir->e.dir, album_linkstring)) {
		return spfs_entity_dir_get_child(album_browse_dir->e.dir, album_linkstring);
	}
	spfs_entity *album_dir = spfs_entity_dir_create(album_linkstring, NULL);
	album_dir->auxdata = album;

	spfs_entity_dir_add_child(album_dir,
			spfs_entity_file_create("name", name_read));

	spfs_entity_dir_add_child(album_browse_dir, album_dir);

	return album_dir;
}
