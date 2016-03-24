#include <check.h>
#include <stdlib.h>
#include "xspf_sanitize.h"

START_TEST(xspf_sanitization)
{
  // Sanitization happends only on the final string containint song/album/artist name.
  const char * source_str = "ampersand: & - single quote: \' - double quote: \" - greater than: > - less than: <";
  const char * dest_str = "ampersand: &amp; - single quote: &apos; - double quote: &quot; - greater than: &gt; - less than: &lt;";
  ck_assert_str_eq(xspf_sanitize(source_str), dest_str);
}
END_TEST

Suite * sanitization_test(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("XSPF Sanitization");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, xspf_sanitization);
    suite_add_tcase(s, tc_core);

    return s;
}

 int main(void)
 {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = sanitization_test();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
