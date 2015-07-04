AM_CFLAGS = -Wall -Werror
AM_CPPFLAGS += -std=c99 -I$(top_srcdir) -D_GNU_SOURCE=1 
ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = spotifile

spotifile_LDADD = $(SPOTIFY_LIBS) $(GLIB_LIBS) $(FUSE_LIBS)
spotifile_CFLAGS = $(SPOTIFY_CFLAGS) $(GLIB_CFLAGS) $(FUSE_CFLAGS)
spotifile_SOURCES = src/spfs_fuse.c \
					src/spfs_spotify.c \
					src/spotify-fs.c src/spotify-fs.h \
					src/appkey.c
					
