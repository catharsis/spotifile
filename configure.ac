AC_INIT([spotifile], m4_esyscmd([build-aux/spfs-version-gen]),
[anton.lofgren@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])
AM_PROG_AR
LT_INIT

AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AM_PROG_CC_C_O

PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([FUSE], [fuse])
PKG_CHECK_MODULES([CHECK], [check])
PKG_CHECK_MODULES([SPOTIFY], [libspotify])

dnl LAME has no pkg-config support, so let's go old-fashioned.
AC_MSG_CHECKING(LAME library)
AC_CHECK_LIB(mp3lame, lame_init, HAVE_LIBLAME=yes, HAVE_LIBLAME=no, )
AC_CHECK_HEADER(lame/lame.h, :, HAVE_LIBLAME=no)

if test "x$HAVE_LIBLAME" = "xyes"; then
	LIBLAME_LIBS="-lmp3lame -lm"
else
	LIBLAME_LIBS=""
fi

AC_SUBST([PACKAGE_VERSION], [AC_PACKAGE_VERSION])
AC_SUBST([HAVE_LIBLAME])
AC_SUBST([LIBLAME_LIBS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_CONFIG_FILES([
	Makefile
	])

AC_OUTPUT
