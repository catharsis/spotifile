#ifndef SPFS_FUSE_UTILS_H
#define SPFS_FUSE_UTILS_H
#include <glib.h>
#define SPFS_SLASH_REPLACEMENT "\u2215"
gchar * spfs_replace_slashes(const gchar *s, const gchar *replacement);
gchar * spfs_sanitize_name(const gchar *n);
#endif /* SPFS_FUSE_UTILS_H */
