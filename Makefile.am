AM_CFLAGS += -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function $(SPOTIFY_CFLAGS) $(GLIB_CFLAGS) $(FUSE_CFLAGS) 
AM_CPPFLAGS += -I$(top_srcdir)/src -D_GNU_SOURCE=1 -DSPOTIFILE_VERSION=\"@PACKAGE_VERSION@\"
bin_PROGRAMS = spotifile
dist_sbin_SCRIPTS = src/scripts/mount.spotifile
check_PROGRAMS = tests_spfs_fuse_test tests_spfs_fuse_utils_test
TESTS = $(check_PROGRAMS)
spotifile_LDADD = $(SPOTIFY_LIBS) $(GLIB_LIBS) $(FUSE_LIBS)
spotifile_SOURCES = src/spfs_fuse.c src/spfs_fuse.h \
					src/spfs_fuse_track.c src/spfs_fuse_track.h \
					src/spfs_fuse_audiofile.c src/spfs_fuse_audiofile.h \
					src/spfs_fuse_artist.c src/spfs_fuse_artist.h \
					src/spfs_fuse_album.c src/spfs_fuse_album.h \
					src/spfs_fuse_playlist.c src/spfs_fuse_playlist.h \
					src/spfs_fuse_utils.c src/spfs_fuse_utils.h \
					src/spfs_fuse_entity.c src/spfs_fuse_entity.h \
					src/spfs_path.c src/spfs_path.h \
					src/spfs_spotify.c src/spfs_spotify.h \
					src/spfs_audio.c src/spfs_audio.h \
					src/spfs_format.c src/spfs_format.h \
					src/spotify-fs.c src/spotify-fs.h \
					src/spfs_appkey.c src/spfs_appkey.h

tests_spfs_fuse_test_SOURCES = tests/spfs_fuse_test.c \
							   src/spfs_fuse_utils.c src/spfs_fuse_utils.h \
							   src/spfs_fuse_entity.c src/spfs_fuse_entity.h \
							   src/spfs_path.c src/spfs_path.h
tests_spfs_fuse_test_LDADD = $(CHECK_LIBS) $(GLIB_LIBS)

tests_spfs_fuse_utils_test_SOURCES = tests/spfs_fuse_utils_test.c \
									 src/spfs_fuse_utils.c src/spfs_fuse_utils.h
tests_spfs_fuse_utils_test_LDADD = $(CHECK_LIBS) $(GLIB_LIBS)
