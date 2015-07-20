#include <glib.h>
#include "spfs_path.h"

/* These functions are very much inspired by relpath.c from coreutils */
size_t spfs_path_common_prefix(const gchar *a, const gchar *b) {
	size_t i = 0, ret = 0;
	g_return_val_if_fail(a != NULL, 0);
	g_return_val_if_fail(b != NULL, 0);
	g_return_val_if_fail(a[0] == '/', 0);
	g_return_val_if_fail(b[0] == '/', 0);

	while (*a && *b) {
		if (*a != *b) {
			break;
		}
		else if (*a == '/') {
			/*found a new common path entity*/
			ret = i + 1;
		}
		a++; b++; i++;
	}

	if ((!*a && !*b) /*Did we find the end of both paths simultaneously? */
			|| (!*a && *b == '/') || (!*b && *a == '/') /* Or one of them, but the other containing a '/' to denote the end of the entity..?*/
			)
		ret = i;

	return ret;
}

gchar *spfs_path_get_relative_path(const gchar *from, const gchar *to) {
	size_t prefix_len = spfs_path_common_prefix(from, to);

	if (prefix_len == 0) {
		g_warning("No common prefix between %s and %s", from, to);
		return NULL;
	}
	const gchar *from_suffix = from + prefix_len;
	const gchar *to_suffix = to + prefix_len;
	GString *relpat = g_string_new(NULL);

	if (*from_suffix) {
		relpat = g_string_append(relpat, "..");
		while (*from_suffix) {
			if (*from_suffix == '/')
				relpat = g_string_append(relpat, "/..");

			++from_suffix;
		}

		if (*to_suffix) {
			g_string_append_printf(relpat, "/%s", to_suffix);
		}
	}
	else {
		if (*to_suffix)
			relpat = g_string_append(relpat, to_suffix);
		else
			relpat = g_string_append(relpat, ".");
	}
	return g_string_free(relpat, FALSE);
}


