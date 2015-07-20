#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include "spfs_fuse_entity.h"
#include "spfs_path.h"


spfs_entity *root;
void setup(void)
{
	root = spfs_entity_root_create(NULL);

	spfs_entity_dir_add_child(root,
			spfs_entity_file_create("foo",
				NULL));

	spfs_entity_dir_add_child(root,
			spfs_entity_file_create("bar",
				NULL));

	spfs_entity *dir = spfs_entity_dir_create("baz", NULL);
	spfs_entity_dir_add_child(dir, spfs_entity_file_create("qux", NULL));
	spfs_entity *dir2 = spfs_entity_dir_create("xyzzy", NULL);
	spfs_entity_dir_add_child(dir2, spfs_entity_link_create("blorf", NULL));

	spfs_entity_dir_add_child(dir, dir2);
	spfs_entity_dir_add_child(root, dir);
}

void teardown(void)
{
	spfs_entity_destroy(root);
}

START_TEST(get_common_prefix)
{
	ck_assert_int_eq(9, spfs_path_common_prefix("/foo/bar/baz", "/foo/bar/bow"));
	ck_assert_int_eq(1, spfs_path_common_prefix("/foo", "/bar"));
	ck_assert_int_eq(1, spfs_path_common_prefix("/foo", "/foobar"));
	ck_assert_int_eq(4, spfs_path_common_prefix("/foo", "/foo/bar"));
	ck_assert_int_eq(5, spfs_path_common_prefix("/foo/", "/foo/bar"));
	ck_assert_int_eq(5, spfs_path_common_prefix("/foo/b", "/foo/bar"));
	ck_assert_int_eq(0, spfs_path_common_prefix("foo/bar", "/foo/bar"));
	ck_assert_int_eq(0, spfs_path_common_prefix(NULL, "/foo/bar"));
	ck_assert_int_eq(0, spfs_path_common_prefix("/foo", NULL));
}
END_TEST

START_TEST(get_relative_path)
{
	struct tc {
		const gchar *from_abspath;
		const gchar *to_abspath;
		const gchar *expected;
	};

	struct tc testcases[] = {
		{"/baz", "/", ".."},
		{"/", "/baz", "baz"},
		{"/baz", "/baz", "."},
		{"/bar/xyzzy/blorf", "/baz", "../../../baz"},
		{"/bar", "/baz/xyzzy/blorf", "../baz/xyzzy/blorf"}
	};

	int i;
	for (i = 0; i < (int)G_N_ELEMENTS(testcases); i++) {
		gchar *actual = spfs_path_get_relative_path(testcases[i].from_abspath, testcases[i].to_abspath);
		ck_assert(actual != NULL);
		ck_assert_str_eq(testcases[i].expected, actual);
		g_free(actual);
	}
}
END_TEST


START_TEST(find_entity)
{
	ck_assert( spfs_entity_find_path(root, "/") != NULL);
	ck_assert( spfs_entity_find_path(root, "/foo") != NULL);
	ck_assert( spfs_entity_find_path(root, "/bar") != NULL);
	ck_assert( spfs_entity_find_path(root, "/baz") != NULL);
	ck_assert( spfs_entity_find_path(root, "/baz/qux") != NULL);
	ck_assert( spfs_entity_find_path(root, "/baz/xyzzy") != NULL);
	ck_assert( spfs_entity_find_path(root, "/baz/xyzzy/blorf") != NULL);
	ck_assert( spfs_entity_find_path(root, "/baz/xyzzy/") != NULL);
	ck_assert( spfs_entity_find_path(root, "/baz////xyzzy/") != NULL);
	ck_assert( spfs_entity_find_path(root, "///baz//xyzzy") != NULL);

	ck_assert( spfs_entity_find_path(root, "/baz/nope") == NULL);
	ck_assert( spfs_entity_find_path(root, "/No") == NULL);
	ck_assert( spfs_entity_find_path(root, "/nuh-uh/") == NULL);
	ck_assert( spfs_entity_find_path(root, "////") == NULL);
	ck_assert( spfs_entity_find_path(root, "//") == NULL);
	ck_assert( spfs_entity_find_path(root, "") == NULL);
}
END_TEST

START_TEST(get_full_path)
{
	const gchar *paths[] = {
		"/",
		"/foo",
		"/bar",
		"/baz",
		"/baz/qux",
		"/baz/xyzzy",
		"/baz/xyzzy/blorf",
		NULL
	};

	const gchar *p = NULL;
	size_t i = 0;
	while ((p = paths[i++])) {
		const gchar *expected;
		char *actual;
		spfs_entity *e = spfs_entity_find_path(root, p);

		expected = p;
		actual = spfs_entity_get_full_path(e);
		ck_assert_str_eq(expected, actual);
		g_free(actual);
	}

	ck_assert(spfs_entity_get_full_path(NULL) == NULL);

}
END_TEST

Suite *
spfs_fuse_suite(void)
{
	Suite *s = suite_create("Spotifile/SpFS FUSE test suite");
	TCase *tc_data_structures= tcase_create("Data structures");
	tcase_add_checked_fixture(tc_data_structures, setup, teardown);
	tcase_add_test(tc_data_structures, find_entity);
	tcase_add_test(tc_data_structures, get_full_path);
	suite_add_tcase(s, tc_data_structures);
	TCase *tc_paths = tcase_create("Paths");
	tcase_add_test(tc_paths, get_common_prefix);
	tcase_add_test(tc_paths, get_relative_path);
	suite_add_tcase(s, tc_paths);
	return s;
}


int
main(void)
{
	int num_failed = 0;
	Suite *s = spfs_fuse_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);
	num_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
