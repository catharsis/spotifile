#include <string.h>
#include <sys/types.h>
#include "spfs_fuse_utils.h"
#include "string_utils.h"

gchar * spfs_replace_slashes(const gchar *s, const gchar *replacement) {
	g_return_val_if_fail(s != NULL, NULL);
	g_return_val_if_fail(replacement != NULL, NULL);
	gchar * new = str_replace(s, "/", replacement);
	while (strstr(new, "/") != NULL) {
		gchar *old = new;
		new = str_replace(new, "/", replacement);
		g_free(old);
	}

	return new;
}

gchar * spfs_sanitize_name(const gchar *n) {
	gchar *san_name = spfs_replace_slashes(n, SPFS_SLASH_REPLACEMENT);
	san_name = g_strstrip(san_name);
	return san_name;
}
