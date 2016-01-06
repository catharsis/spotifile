#ifndef SPFS_PATH_H
#define SPFS_PATH_H
/**
 * Calculate the length of the longest common prefix of paths \p a and
 * \p b
 * @param[in] a,b The paths to calculate the common prefix for
 * @return The length, in bytes, of the longest common prefix
 */
size_t spfs_path_common_prefix(const gchar *a, const gchar *b);

/**
 * Calculate the relative path from path \p from to path \p to
 * @param[in] from The origin path
 * @param[in] to   The target path
 * @return A newly-allocated string containing the relative path, or NULL if no
 * valid path is possible
 */
gchar *spfs_path_get_relative_path(const gchar *from, const gchar *to);
#endif /* SPFS_PATH_H */
