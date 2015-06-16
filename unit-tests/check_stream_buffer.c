#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ulog.h"
#include "util.h"

#include "../src/stream_buffer.h"

START_TEST(test_buffer_create)
{
  struct Stream_Buffer *buf = NULL;
  buf = stream_buffer_new();
  fail_if(buf == NULL);
  fail_if(buf->head == NULL);
  fail_unless(buf->head == buf->tail);
  stream_buffer_delete(buf);
}
END_TEST

START_TEST(test_buffer_add)
{
  size_t strlen = BUFFER_MAX;
  char *str = malloc(sizeof(char) * strlen+1);
  struct Stream_Buffer *buf = NULL;

  buf = stream_buffer_new();
  for ( int i = 0; i < 100; i++ ) {
    memset(str, 'A' + i, strlen);
    fail_unless(stream_buffer_add(buf, str, strlen) == strlen);
    fail_unless(stream_buffer_add(buf, str, strlen) == strlen);
    fail_unless(buf->curr_size = strlen * 2);
    fail_unless(buf->tail == buf->head + buf->curr_size);
    fail_unless(buf->curr_size == (buf->tail - buf->head));

    stream_buffer_clear(buf);
    fail_unless(buf->curr_size == 0);
    fail_unless(buf->head == buf->tail);
  }

  free(str);
  stream_buffer_delete(buf);
}  
END_TEST


#define TEST_STRING "abcdefghijklmnopqrstuvwxyz.ABCDEFGHIJKLMNOPQRSTUVWXYZ.\n\1\n2\n3\n"
START_TEST(test_buffer_clear)
{
  struct Stream_Buffer *buf = stream_buffer_new();

  size_t length = strlen(TEST_STRING);
  fail_unless(stream_buffer_add(buf, TEST_STRING, length) == length);

  stream_buffer_delete(buf);
}
END_TEST


int do_test_nibble(struct Stream_Buffer *buf, const int nibble, const int original_len, const int written_so_far)
{
#ifdef CHECK_STREAM_BUFFER_DEBUG
  printf("Next %d chars: ", nibble);
  fflush(stdout);
#endif
  stream_buffer_write_to(buf, STDOUT_FILENO, nibble);
  fail_unless(buf->curr_size = original_len - written_so_far - nibble);
#ifdef CHECK_STREAM_BUFFER_DEBUG
  printf("\t[%d - %d - %d = %zd; next 3 chars=%c%c%c%c%c%c%c]\n", original_len, written_so_far, nibble, buf->curr_size, 
	 (buf->curr_size > 0 ? buf->head[0] : '-'),
	 (buf->curr_size > 1 ? buf->head[1] : '-'),
	 (buf->curr_size > 3 ? buf->head[2] : '-'),
	 (buf->curr_size > 4 ? buf->head[3] : '-'),
	 (buf->curr_size > 5 ? buf->head[4] : '-'),
	 (buf->curr_size > 6 ? buf->head[5] : '-'),
	 (buf->curr_size > 7 ? buf->head[6] : '-') );
  fflush(stdout);
#endif
  return nibble;
}


START_TEST(test_buffer_partial_write)
{
  struct Stream_Buffer *buf = stream_buffer_new();
  int len = strlen(TEST_STRING);
  int half = len / 2;

  printf("Test string is %d (half=%d) characters. Expected:\n%s\nGot:\n", len, half, TEST_STRING);
  stream_buffer_add(buf, TEST_STRING, len);
  fail_unless(buf->curr_size == len);

  int written = 0;
  written += do_test_nibble(buf, 3, len, written);
  fail_unless( strncmp(&(TEST_STRING[written]), buf->head, len - written) == 0 );

  written += do_test_nibble(buf, 1, len, written);
  fail_unless( strncmp(&(TEST_STRING[written]), buf->head, len - written) == 0 );

  written += do_test_nibble(buf, half, len, written);
  fail_unless( strncmp(&(TEST_STRING[written]), buf->head, len - written) == 0 );

  written += do_test_nibble(buf, buf->curr_size - 1, len, written);
  fail_unless( strncmp(&(TEST_STRING[written]), buf->head, len - written) == 0 );
  fail_unless(buf->curr_size == 1);

  stream_buffer_write(buf, STDOUT_FILENO);
  fail_unless(buf->curr_size == 0);
  printf("\n");

  stream_buffer_delete(buf);
}
END_TEST


Suite *stream_buffer_suite(void)
{
  Suite *s = suite_create("Stream_Buffer");

  // Core test case 
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_buffer_create);
  tcase_add_test(tc_core, test_buffer_add);
  tcase_add_test(tc_core, test_buffer_clear);
  suite_add_tcase(s, tc_core);

  // Partial buffer writes
  TCase *tc_partials = tcase_create("Partial Writes");
  tcase_add_test(tc_partials, test_buffer_partial_write);
  suite_add_tcase(s, tc_partials);

  return s;
}


int main(int argc, char *argv[]) 
{
  int number_failed = 0;
  Suite   *s  = stream_buffer_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
