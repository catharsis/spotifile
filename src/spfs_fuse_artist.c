#include "spfs_fuse_artist.h"
#include "spfs_fuse_album.h"
#include "spfs_spotify.h"

static int name_read(struct fuse_file_info *fi, char *buf, size_t size, off_t offset) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	char *str = g_strdup_printf("%s\n", spotify_artist_name(
				spotify_artistbrowse_artist(e->parent->auxdata)
				));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int portrait_read(struct fuse_file_info *fi, char *buf, size_t size, off_t offset) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	sp_image *image = e->auxdata;
	size_t image_size;
	void * image_data = spotify_image_data(image, &image_size);
	READ_SIZED_OFFSET((char *)image_data, image_size, buf, size, offset);
	g_free(image_data);
	return size;
}

static int portraits_readdir(struct fuse_file_info *fi, void *buf, fuse_fill_dir_t filler, off_t offset) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	GArray *portraits = spotify_get_artistbrowse_portraits(e->parent->auxdata);
	if (!portraits)
		return 0;

	for (guint i = 0; i < portraits->len; ++i) {
		gchar *portrait_name = g_strdup_printf("%d.jpg", i+1);

		if (!spfs_entity_dir_has_child(e->e.dir, portrait_name)) {
			spfs_entity *portrait_file = spfs_entity_file_create(portrait_name,
					&(struct spfs_file_ops){.read = portrait_read}
					);
			portrait_file->auxdata = spotify_image_create(
					SPFS_SP_SESSION,
					g_array_index(portraits, const byte *, i)
					);
			spfs_entity_dir_add_child(e, portrait_file);
		}

		g_free(portrait_name);
	}
	return 0;
}

static int albums_readdir(struct fuse_file_info *fi, void *buf, fuse_fill_dir_t filler, off_t offset) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	GArray *albums = spotify_get_artistbrowse_albums(e->parent->auxdata);
	if (!albums)
		return 0;

	for (guint i = 0; i < albums->len; ++i) {
		sp_album *album = g_array_index(albums, sp_album *, i);

		spfs_entity *album_browse_dir = create_album_browse_dir(album);
		gchar *album_name = spotify_album_name(album);
		/* FIXME: Deal with duplicates (re-releases, region availability.) properly
		 * Probably by presenting the album that has the most tracks available
		 * to the user as suggested here: http://stackoverflow.com/a/11994581
		 *
		 * XXX: Perhaps, it'd be a good idea to also add the release year to the album title
		 * like "Ride The Lightning (1984)" to further differentiate between re-releases
		 */
		if (!spfs_entity_dir_has_child(e->e.dir, album_name)) {
			spfs_entity *album_link = spfs_entity_link_create(album_name, NULL);
			spfs_entity_dir_add_child(e, album_link);
			gchar *rpath = relpath(e, album_browse_dir);
			spfs_entity_link_set_target(album_link, rpath);
			g_free(rpath);
		}
		g_free(album_name);
	}
	g_array_free(albums, false);
	return 0;
}

static int biography_read(struct fuse_file_info *fi, char *buf, size_t size, off_t offset) {
	spfs_entity *e = SPFS_FH2ENT(fi->fh);
	char *str = spotify_artistbrowse_biography(e->parent->auxdata);
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

spfs_entity *create_artist_browse_dir(sp_artist *artist) {
	g_return_val_if_fail(artist != NULL, NULL);
	spfs_entity *artist_browse_dir = spfs_entity_find_path(SPFS_DATA->root, "/browse/artists");
	sp_link *link = spotify_link_create_from_artist(artist);

	g_return_val_if_fail(link != NULL, NULL);
	gchar artist_linkstring[1024] = {0, };
	spotify_link_as_string(link, artist_linkstring, 1024);
	if (spfs_entity_dir_has_child(artist_browse_dir->e.dir, artist_linkstring)) {
		return spfs_entity_dir_get_child(artist_browse_dir->e.dir, artist_linkstring);
	}
	spfs_entity *artist_dir = spfs_entity_dir_create(artist_linkstring, NULL);
	artist_dir->auxdata = spotify_artistbrowse_create(SPFS_SP_SESSION, artist);

	spfs_entity_dir_add_child(artist_dir,
			spfs_entity_file_create("name",
				&(struct spfs_file_ops){.read = name_read})
			);

	spfs_entity_dir_add_child(artist_dir,
			spfs_entity_file_create("biography",
				&(struct spfs_file_ops){.read = biography_read})
			);

	spfs_entity_dir_add_child(artist_dir,
			spfs_entity_dir_create("albums",
				&(struct spfs_dir_ops){.readdir = albums_readdir})
			);

	spfs_entity_dir_add_child(artist_dir,
			spfs_entity_dir_create("portraits",
				&(struct spfs_dir_ops){.readdir = portraits_readdir})
			);

	spfs_entity_dir_add_child(artist_browse_dir, artist_dir);
	return artist_dir;
}


