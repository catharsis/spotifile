#ifndef SPFS_PATH_H
#define SPFS_PATH_H
size_t spfs_path_common_prefix(const gchar *a, const gchar *b);
gchar *spfs_path_get_relative_path(const gchar *from, const gchar *to);
#endif /* SPFS_PATH_H */
