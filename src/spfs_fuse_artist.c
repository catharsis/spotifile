#include "spfs_fuse_artist.h"
#include "spfs_spotify.h"

static int name_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
	char *str = g_strdup(spotify_artist_name(
				spotify_artistbrowse_artist(e->parent->auxdata)
				));
	READ_OFFSET(str, buf, size, offset);
	g_free(str);
	return size;
}

static int biography_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	spfs_entity *e = (spfs_entity *)fi->fh;
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
			spfs_entity_file_create("name", name_read));

	spfs_entity_dir_add_child(artist_dir,
			spfs_entity_file_create("biography", biography_read));

	spfs_entity_dir_add_child(artist_browse_dir, artist_dir);
	return artist_dir;
}


