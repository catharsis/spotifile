#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include "xspf.h"
#define XML_DECL "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
START_TEST(build_xspf)
{
	xspf *x = xspf_new();
	char *actual = NULL;
	const char *expected = XML_DECL
		"<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n"
		"</playlist>\n";
	x = xspf_begin_playlist(x);
	x = xspf_end_playlist(x);
	actual = xspf_free(x, false);
	ck_assert_str_eq(expected, actual);
	free(actual);

	expected = XML_DECL
		"<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n"
		"<trackList>\n"
		"</trackList>\n"
		"</playlist>\n";
	x = xspf_new();
	x = xspf_begin_playlist(x);
	x = xspf_begin_tracklist(x);
	x = xspf_end_tracklist(x);
	x = xspf_end_playlist(x);
	actual = xspf_free(x, false);
	ck_assert_str_eq(expected, actual);
	free(actual);

	expected = XML_DECL
		"<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n"
		"<trackList>\n"
		"<track>\n"
		"</track>\n"
		"<track>\n"
		"</track>\n"
		"</trackList>\n"
		"</playlist>\n";
	x = xspf_new();
	x = xspf_begin_playlist(x);
	x = xspf_begin_tracklist(x);
	x = xspf_begin_track(x);
	x = xspf_end_track(x);
	x = xspf_begin_track(x);
	x = xspf_end_track(x);
	x = xspf_end_tracklist(x);
	x = xspf_end_playlist(x);
	actual = xspf_free(x, false);
	ck_assert_str_eq(expected, actual);
	free(actual);

}
END_TEST

Suite *
xspf_suite(void)
{
	Suite *s = suite_create("XSPF test suite");
	TCase *tc_xspf = tcase_create("XSPF Building");
	tcase_add_test(tc_xspf, build_xspf);
	suite_add_tcase(s, tc_xspf);
	return s;
}

int
main(void)
{
	int num_failed = 0;
	Suite *s = xspf_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);
	num_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
