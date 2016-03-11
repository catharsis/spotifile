#include <string.h>
#include "xspf_sanitize.h"
#include "string_utils.h"
#include <stdio.h>

const gchar * replace_all(const gchar * str, const gchar *key, const gchar * val) {
    gchar * tmpstr = str_replace(str, key, val);
    while (strstr(tmpstr, key) != NULL) {
        gchar *old = tmpstr;
        tmpstr = str_replace(tmpstr, key, val);
        g_free(old);
    }

    return tmpstr;
}

gchar * xspf_escape_ampersand(const gchar * s) {
    gchar * tmpstr = str_replace(s, "&", "!amp;");
    replace_all(tmpstr, "!amp;", "&amp;");
    return tmpstr;
}
