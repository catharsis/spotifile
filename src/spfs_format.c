#include "spfs_format.h"
bool spfs_format_isvalid(const gchar *fmt)
{
	g_return_val_if_fail(fmt != NULL, false);
	const gchar *tmp = fmt;

	bool valid = true;

	while (tmp && *tmp != '\0') {
		switch (*tmp) {
			case '/': valid = false; break;
			default: ++tmp; break;
		}
	}
	if (fmt >= tmp)
		valid = false;

	return valid;
}

GString * spfs_format_playlist_track_expand(const gchar *fmt, sp_track *track, int index) {
	g_return_val_if_fail(fmt != NULL, NULL);
	g_return_val_if_fail(track != NULL, NULL);
	if (index <= 0) {
		g_warning("Invalid playlist track index %d, using 1 instead", index);
		index = 1;
	}
	GString *out = g_string_new(NULL);
	bool specifier_found = false;
	while (*fmt != '\0') {
		if (specifier_found) {
			switch (*fmt) {
				case 'a':
					out = g_string_append(out, spotify_artist_name(
								spotify_track_artist(track, 0)
								));
					break;
				case 'i':
					g_string_append_printf(out, "%.2d", index);
					break;
				case 't':
					out = g_string_append(out, spotify_track_name(track));
					break;
				default:
					out = g_string_append_c(out, '%');
					out = g_string_append_c(out, *fmt);
			}
			specifier_found = false;
		}
		else {
			switch (*fmt) {
				case '%':
					specifier_found = true;
					break;
				default:
					out = g_string_append_c(out, *fmt);

			}
		}

		++fmt;
	}
	out = g_string_append(out, ".wav");
	return out;
}
