#include <check.h>
#include <stdlib.h>
#include <string.h>

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
    fail_unless(stream_buffer(buf, str, strlen) == strlen);
    fail_unless(stream_buffer(buf, str, strlen) == strlen);
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

Suite *stream_buffer_suite(void)
{
  Suite *s = suite_create("Stream_Buffer");

  /* Core test case */
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_buffer_create);
  tcase_add_test(tc_core, test_buffer_add);
  suite_add_tcase (s, tc_core);
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
