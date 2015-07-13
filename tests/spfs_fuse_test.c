#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include "spfs_fuse_entity.h"


spfs_entity *root;
void setup(void)
{
	root = spfs_entity_root_create(NULL, NULL);

	spfs_entity_dir_add_child(root,
			spfs_entity_file_create("foo",
				NULL,
				NULL));

	spfs_entity_dir_add_child(root,
			spfs_entity_file_create("bar",
				NULL,
				NULL));

	spfs_entity *dir = spfs_entity_dir_create("baz", NULL, NULL);
	spfs_entity_dir_add_child(dir, spfs_entity_file_create("qux", NULL, NULL));
	spfs_entity *dir2 = spfs_entity_dir_create("xyzzy", NULL, NULL);
	spfs_entity_dir_add_child(dir2, spfs_entity_link_create("blorf", NULL, NULL));

	spfs_entity_dir_add_child(dir, dir2);
	spfs_entity_dir_add_child(root, dir);
}

void teardown(void)
{
	spfs_entity_destroy(root);
}

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
