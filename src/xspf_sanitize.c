#include <string.h>
#include "xspf_sanitize.h"
#include "string_utils.h"
#include <stdio.h>

gchar * replace_all(const gchar * str, const gchar *key, const gchar * val) {
    gchar * tmpstr = str_replace(str, key, val);
    while (strstr(tmpstr, key) != NULL) {
        gchar *old = tmpstr;
        tmpstr = str_replace(tmpstr, key, val);
        g_free(old);
    }

    return tmpstr;
}

// be sure to esacpe whatever needs to be escaped in XML format
// " ->  &quot;
// ' ->  &apos;
// < ->  &lt;
// > -> &gt;
// & -> &amp;

gchar * xspf_escape_double_quote(const gchar * s) {
    return replace_all(s, "\"", "&quot;");
}

gchar * xspf_escape_single_quote(const gchar * s) {
    return replace_all(s, "\'", "&apos;");
}

gchar * xspf_escape_lt(const gchar * s) {
    return replace_all(s, "<", "&lt;");
}

gchar * xspf_escape_gt(const gchar * s) {
    return replace_all(s, ">", "&gt;");
}

gchar * xspf_escape_ampersand(const gchar * s) {
    gchar * tmpstr = str_replace(s, "&", "!amp;");
    return replace_all(tmpstr, "!amp;", "&amp;");
}

gchar * xspf_sanitize(const gchar * s) {
    gchar * tmp = xspf_escape_ampersand(s);
    if(strstr(tmp, "\"") != NULL) {
        tmp = xspf_escape_double_quote(tmp);
    }

    if(strstr(tmp, "\'") != NULL) {
        tmp = xspf_escape_single_quote(tmp);
    }

    if(strstr(tmp, "<") != NULL) {
        tmp = xspf_escape_lt(tmp);
    }

    if (strstr(tmp, ">") != NULL) {
        tmp = xspf_escape_gt(tmp);
    }

    return tmp;
}
