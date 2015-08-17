#include "spfs_fuse_album.h"
#include "spfs_spotify.h"
static int name_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	gchar *str = g_strdup_printf("%s\n", spotify_album_name(e->parent->auxdata));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int cover_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	const byte *image_id = spotify_album_cover(e->parent->auxdata, SP_IMAGE_SIZE_NORMAL);

	sp_image *image = spotify_image_create(SPFS_SP_SESSION, image_id);
	size_t image_size;
	void * image_data = spotify_image_data(image, &image_size);
	READ_SIZED_OFFSET((char *)image_data, image_size, buf, size, offset);
	g_free(image_data);
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
			spfs_entity_file_create("name",
				&(struct spfs_file_ops) {.read = name_read}));

	spfs_entity_dir_add_child(album_dir,
			spfs_entity_file_create("cover.jpg",
				&(struct spfs_file_ops) {.read = cover_read}));

	spfs_entity_dir_add_child(album_browse_dir, album_dir);

	return album_dir;
}
