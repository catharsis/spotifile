#include "string_utils.h"
#include <string.h>
#include <sys/types.h>

gchar *str_replace(const gchar *s, const gchar *c, const gchar *replacement) {
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
