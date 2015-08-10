#include <string.h>
#include "spfs_fuse_utils.h"
static gchar *str_replace(const gchar *s, const gchar *c, const gchar *replacement) {
	size_t s_l = strlen(s);
	size_t r_l = strlen(replacement);
	size_t c_l = strlen(c);
	gchar *new = g_malloc0(s_l + r_l + 1);
	gchar *p = strstr(s, c);
	if (!p) {
		strcpy(new, s);
		return new;
	}

	off_t off = p - s;
	memcpy(new, s, off);
	memcpy(new + off, replacement, r_l);
	memcpy(new + off + r_l, s + off + c_l, s_l - c_l - off);
	new[s_l + r_l] = '\0';
	return new;
}

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
