#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ulog.h"

#include "util.h"

#define MAX_DST 16
#define TEST_STRING "12345678901234567890"

START_TEST(test_strlcat)
{
  int src_len = 0;
  size_t ncat = 0;
  char dst[MAX_DST];

  /* Sanity check our own test environment */
  src_len = strlen(TEST_STRING);
  fail_unless(src_len > MAX_DST);
  ck_assert_str_eq(TEST_STRING, TEST_STRING);

  /* Test that copying part of a string only copies part, but that truncation detectable */
  dst[0] = '\0';
  ncat = strlcat(dst, TEST_STRING, 8);
  ck_assert_str_eq(dst, "1234567");
  ck_assert_int_eq(ncat, src_len);
  ck_assert_int_ne(ncat, 7);

  /* Test that copying too large a string will truncate */
  dst[0] = '\0';
  ncat = strlcat(dst, TEST_STRING, MAX_DST);
  ck_assert_str_eq(dst, "123456789012345");
  ck_assert_int_eq(ncat, src_len);
  fail_unless(ncat > MAX_DST);
}
END_TEST

START_TEST(test_upper_power_of_two)
{
  /* check function */
  static size_t test_pairs[][2] = 
    {
      { 0, 2 },
      { 1, 2 },
      { 7, 8 },
      { 100, 128 },
      { 1023, 1024 },
      { 1024, 1024 },
      { 1025, 2048 },
      { 32 * 1024, 32 * 1024 },
      { (32 * 1024) - 1, 32 * 1024 },
      { (32 * 1024) + 1, 64 * 1024 },
      { 64 * 1024, 64 * 1024 },
      { (64 * 1024) - 1, 64 * 1024 },
      { (64 * 1024) + 1, 128 * 1024 },
      { 0, 2 },
      { -1, -1 }
    };

  int i = 0;
  while ( test_pairs[i][0] != -1 ) {
    size_t testval  = test_pairs[i][0];
    size_t expected = test_pairs[i][1];
    size_t actual   = upper_power_of_two(testval);

    fail_unless( actual == expected );
    i++;
  }

  /* TODO: test overflow behaviour */

  /* checks that BUFFER_MAX is defined as a power of 2 */
  fail_unless(upper_power_of_two(BUFFER_MAX) == BUFFER_MAX);
}
END_TEST

Suite *util_suite(void)
{
  Suite *s = suite_create("util_suite");

  /* Core test case */
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_strlcat);
  tcase_add_test(tc_core, test_upper_power_of_two);
  suite_add_tcase(s, tc_core);
  return s;
}


int main(int argc, char *argv[]) 
{
  int number_failed = 0;
  Suite   *s  = util_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_ENV);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
