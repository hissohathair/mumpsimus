#include <assert.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ulog.h"
#include "util.h"

#include "../src/pipes.h"

#ifdef NDEBUG
#error "Do not run unit tests with NDEBUG defined"
#endif

#define TEST_COMMAND  "/usr/bin/tr '[:lower:]' '[:upper:]'"
#define TEST_STRING   "ABCDEFGHIJKLMNOPQRSTUVWXYZ.abcdefghijklmnopqrstuvwxyz <-- expect all caps\n"
#define EXPECT_STRING "ABCDEFGHIJKLMNOPQRSTUVWXYZ.ABCDEFGHIJKLMNOPQRSTUVWXYZ <-- EXPECT ALL CAPS\n"


/* test_pipe_create: Tests basic initialisation of handle struct.
 */
START_TEST(test_pipe_create)
{
  struct Pipe_Handle *ptr_ph = NULL;

  // Test with NULL argument (should allocate new structure)
  ptr_ph = pipe_handle_new();
  fail_if(ptr_ph == NULL);
  fail_unless(PIPES_CLOSED == ptr_ph->state);
  fail_if(ptr_ph->command_line == NULL);

  // Free struture
  pipe_handle_delete(ptr_ph);
}
END_TEST


/* test_unidrectional_pipe: Test writing to uni-pipe.
 */
START_TEST(test_unidirectional_pipe)
{
  struct Pipe_Handle *ph = pipe_handle_new();
  assert(ph != NULL);

  // Open a pipe to cat for testing
  fail_unless(0 == pipe_open(ph, TEST_COMMAND));
  fail_unless(PIPES_OPEN_UNI == ph->state);
  fail_unless(ph->child_pid >= 1);

  // Write some data -- should appear on stdout
  size_t length = strlen(TEST_STRING);
  ssize_t bytes = write(pipe_write_fileno(ph), TEST_STRING, length);
  fail_unless(bytes == length);

  // Close pipe
  fail_unless(0 == pipe_close(ph));
  fail_unless(PIPES_CLOSED == ph->state);

  pipe_handle_delete(ph);
}
END_TEST


/* test_undirectional_reset: Test resetting a uni-pipe.
 */
START_TEST(test_unidirectional_reset)
{
  struct Pipe_Handle *ph = pipe_handle_new();
  assert(ph != NULL);

  // Open a pipe to cat for testing
  fail_unless(0 == pipe_open(ph, TEST_COMMAND));
  fail_unless(PIPES_OPEN_UNI == ph->state);
  fail_unless(ph->child_pid >= 1);

  // Write some data -- should appear on stdout
  size_t length = strlen(TEST_STRING);
  ssize_t bytes = write(pipe_write_fileno(ph), TEST_STRING, length);
  fail_unless(bytes == length);

  // Reset the pipe
  fail_unless(0 == pipe_reset(ph));

  // Write some more data
  bytes = write(pipe_write_fileno(ph), TEST_STRING, length);
  fail_unless(bytes == length);

  // Close the pipe
  fail_unless(0 == pipe_close(ph));
  fail_unless(PIPES_CLOSED == ph->state);

  pipe_handle_delete(ph);
}
END_TEST


void run_read_write_tests_on_bipipe(struct Pipe_Handle *ph)
{
  assert(ph != NULL);

  size_t length = strlen(TEST_STRING);
  ssize_t bytes = write(pipe_write_fileno(ph), TEST_STRING, length);
  fail_unless(bytes == length);

  // Close our write end
  pipe_send_eof(ph);
  fail_unless(PIPES_WIDOWED == ph->state);

  // Now read some data back
  char *buffer = malloc(length+1);
  assert(buffer != NULL);
  bytes = read(pipe_read_fileno(ph), buffer, length);
  if ( bytes < 0 ) {
    perror("Error reading from pipe");
  }
  fail_unless(bytes == length);
  fail_unless(strncmp(buffer, EXPECT_STRING, length) == 0);

  free(buffer);
}


/* test_bipipe_create: Test reading and writing a bi-pipe.
 */
START_TEST(test_bipipe_create)
{
  struct Pipe_Handle *ph = pipe_handle_new();
  assert(ph != NULL);

  // Open a bi-directional pipe
  fail_unless(0 == pipe_open2(ph, TEST_COMMAND));
  fail_unless(PIPES_OPEN_BI == ph->state);
  fail_unless(ph->child_pid >= 1);

  // Write some data to pipe's "stdin"
  run_read_write_tests_on_bipipe(ph);

  // Close the pipe
  fail_unless(0 == pipe_close(ph));
  fail_unless(PIPES_CLOSED == ph->state);

  pipe_handle_delete(ph);
}
END_TEST


/* test_bipipe_reset: Test resetting a bi-pipe.
 */
START_TEST(test_bipipe_reset)
{
  struct Pipe_Handle *ph = pipe_handle_new();
  assert(ph != NULL);

  // Open a bi-directional pipe
  fail_unless(0 == pipe_open2(ph, TEST_COMMAND));
  fail_unless(PIPES_OPEN_BI == ph->state);
  fail_unless(ph->child_pid >= 1);

  // Write some data to pipe's "stdin"
  run_read_write_tests_on_bipipe(ph);

  // Reset the pipe
  fail_unless(0 == pipe_reset(ph));

  // Re-run those tests
  run_read_write_tests_on_bipipe(ph);

  // Close the pipe
  fail_unless(0 == pipe_close(ph));
  fail_unless(PIPES_CLOSED == ph->state);

  pipe_handle_delete(ph);
}
END_TEST


Suite *pipes_suite(void)
{
  Suite *s = suite_create("Pipes");

  // Core test case
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_pipe_create);
  tcase_add_test(tc_core, test_unidirectional_pipe);
  tcase_add_test(tc_core, test_unidirectional_reset);
  suite_add_tcase(s, tc_core);


  // Bi-directional test case
  TCase *tc_bidi = tcase_create("Bi-Directional");
  tcase_add_test(tc_bidi, test_bipipe_create);
  tcase_add_test(tc_bidi, test_bipipe_reset);
  suite_add_tcase(s, tc_bidi);

  return s;
}


int main(int argc, char *argv[]) 
{
  int number_failed = 0;
  Suite   *s  = pipes_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
