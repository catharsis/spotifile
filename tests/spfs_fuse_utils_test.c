#include <check.h>
#include <stdlib.h>
#include "spfs_fuse_utils.h"


START_TEST(replace_slashes)
{
	gchar *str = spfs_replace_slashes("", "");
	ck_assert_str_eq("", str);
	g_free(str);

	str = spfs_replace_slashes("/", ":");
	ck_assert_str_eq(":", str);
	g_free(str);

	str = spfs_replace_slashes("//", ":");
	ck_assert_str_eq("::", str);
	g_free(str);

	str = spfs_replace_slashes("//", "foo");
	ck_assert_str_eq("foofoo", str);
	g_free(str);

	str = spfs_replace_slashes("/./", ":");
	ck_assert_str_eq(":.:", str);
	g_free(str);

	str = spfs_replace_slashes("/", "\u2215");
	ck_assert_str_eq("\u2215", str);
	g_free(str);

	str = spfs_replace_slashes("-/-", "\u2215");
	ck_assert_str_eq("-\u2215-", str);
	g_free(str);

	str = spfs_replace_slashes("------------------/-", "\u2215");
	ck_assert_str_eq("------------------\u2215-", str);
	g_free(str);
}
END_TEST

START_TEST(sanitize_name)
{
	gchar *str = spfs_sanitize_name("Jack and The Wild Horses - S/T");
	ck_assert_str_eq("Jack and The Wild Horses - S" SPFS_SLASH_REPLACEMENT "T", str);
	g_free(str);

	str = spfs_sanitize_name(" AC/DC ");
	ck_assert_str_eq("AC" SPFS_SLASH_REPLACEMENT "DC", str);
	g_free(str);
}
END_TEST

Suite *
spfs_fuse_utils_suite(void)
{
	Suite *s = suite_create("Spotifile/SpFS FUSE utilities test suite");
	TCase *tc_strings = tcase_create("Strings and names");
	tcase_add_test(tc_strings, replace_slashes);
	tcase_add_test(tc_strings, sanitize_name);
	suite_add_tcase(s, tc_strings);
	return s;
}

int
main(void)
{
	int num_failed = 0;
	Suite *s = spfs_fuse_utils_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);
	num_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
