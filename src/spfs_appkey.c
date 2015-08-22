#include "spfs_appkey.h"
#include <glib.h>

static const gchar *k64 = "Ad4CRJ9R5nFgntpWPfSX7azcrDN/VenS4b33v2DpK1iYXSH6g6DJFipjeOIk7vTpZtQwCjUVrTpkCHy172zFJE8duEoTMj1A9tbMlyg4AAIYKOylKqrU701r6iPzs2lqN4Jd6gQRwbT+meBhWGUxbpinvuldQeTiIqArKwvyc2Igqj5hs75FaHWOIUH/ee4EEDeueGPQtRo6BD2WndNkaNyUCx1NwFerfsMxRBmBKuuHN50KOAw863jFK2DjNStd4kQLiFOpT+EdI8Apb5L4vsUZGsYD+eUiWteP63gga5FKSr4dtibVS2cLtNLfC8cWN4jWCYnkxCytPJAEl24Xzpgk+aArx3XsSE/d6Ej/tqBS5pxdfL/irgHd9bEOuF+AXnzD3TwRRmMF6KsNONBDcIWF/KfcpirzEgVTohAZtFUj";

void * spfs_appkey_get(size_t *ksz) {
	return g_base64_decode(k64, (gsize *) ksz);
}

