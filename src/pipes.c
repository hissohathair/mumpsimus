/* pipes.c: Library for handling pipes.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <assert.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef HAVE_SYS_SYSLIMITS_H
#  include <sys/syslimits.h>
#endif
#ifdef HAVE_LINUX_LIMITS_H
#  include <linux/limits.h>
#endif

#include "pipes.h"
#include "ulog.h"


/* pipe_handle_new:
 *
 *    Allocates memory for a new pipe handle and Initialises the
 *    internal structure.  Returns NULL if malloc fails.
 */
struct Pipe_Handle *
pipe_handle_new (void)
{
  // Allocate memory for handle
  struct Pipe_Handle *ph = malloc (sizeof (struct Pipe_Handle));

  // Initialise fields (incl malloc for pipe_cmd)
  if (NULL != ph)
  {
    ph->state = PIPES_CLOSED;
    ph->child_pid = 0;
    ph->pipe_fds[0] = STDIN_FILENO;
    ph->pipe_fds[1] = STDOUT_FILENO;
    ph->bipipe_fds[0] = 0;
    ph->bipipe_fds[1] = 0;

    ph->command_line = malloc (ARG_MAX);
    if (NULL == ph->command_line)
    {
      free (ph);
      ph = NULL;
    }
  }
  return ph;
}

void
pipe_handle_delete (struct Pipe_Handle *ph)
{
  assert (ph != NULL);

  // Shutdown any open pipes
  if (PIPES_CLOSED != ph->state)
    pipe_close (ph);

  // Free internally allocated memory
  free (ph->command_line);
  ph->command_line = NULL;

  // Free handle
  free (ph);

  return;
}


/* pipe_open: 
 *
 *    Open a unidirectional pipe to new process specified by
 *    "command_line".  Forks. Child will exec "command_line", via
 *    "/bin/sh -c". When parent writes to pipe_fileno(ph), the new
 *    child will read that on its stdin. Anything the new child writes
 *    to stdout will appear on parent's stdout.
 *
 *    Paramater "command_line" is copied internally -- callers do not
 *    need to worry about keeping the string in memory.
 *
 *    Will update the pipe handle "ph" with new state
 *    information. Returns 0 on success, non-zero on error.
 */
int
pipe_open (struct Pipe_Handle *ph, const char *command_line)
{
  // Expect handle initialised and pipe not opened yet.
  assert (ph != NULL);
  assert (PIPES_CLOSED == ph->state);

  // Copy this for later. 
  strncpy (ph->command_line, command_line, ARG_MAX);

  // Create pipe channel. Parent will send data to child. Child will send data to same stdout as parent.
  if (pipe (ph->pipe_fds) == -1)
  {
    perror ("Unable to create pipe");
    return -1;
  }


  /* Now we have a unidirectional pipe set up in pipe_fds. Data
   * written to pipe_fds[1] appears on (ie can be read from)
   * pipe_fds[0].
   *
   * So we we want is for parent to write to pipe_fds[1] and the
   * child will read this on its version of pipe_fds[0].
   */
  ph->state = PIPES_OPENING;


  // Fork here. 
  ph->child_pid = fork ();
  if (-1 == ph->child_pid)
  {
    perror ("Unable to fork");
    return -1;
  }


  /* Parent will continue normally. Child will set up file handles and
   * exec.
   */

  if (0 == ph->child_pid)
  {
    // Am child. I receive data, so don't need the write end of pipe. 
    close (ph->pipe_fds[1]);

    /* Child inherits descriptors, which point at the same objects. We
     * want the child's stdin however to be the fd that the parent is
     * writing to. Leave stdout pointing at parent's stdout.
     */
    if (dup2 (ph->pipe_fds[0], STDIN_FILENO) != STDIN_FILENO)
    {
      perror ("Could not dup2 pipe to stdin");
      abort ();			// TODO: This is just child exiting -- how will parent know?
    }

    // Exec here
    ulog (LOG_INFO, "Child has set up its end. About to exec: %s -c %s",
	  _PATH_BSHELL, ph->command_line);
    if (execl (_PATH_BSHELL, _PATH_BSHELL, "-c", ph->command_line, NULL) ==
	-1)
    {
      ulog (LOG_ERR, "Child was not able to exec: %s -c %s", _PATH_BSHELL,
	    ph->command_line);
      perror ("Error in execl");
      abort ();			// TODO: Again, this is just child. Parent needs to know.
    }

    // Child should never get here!
    perror ("Unreachable code reached!");
    abort ();
  }
  else
  {
    // Am parent. I send data, so don't need read end of pipe.
    close (ph->pipe_fds[0]);
    ulog (LOG_INFO, "Parent has set up its end. Child is pid=%d. Pipe fd=%d ",
	  ph->child_pid, ph->pipe_fds[1]);
  }

  ph->state = PIPES_OPEN_UNI;
  return 0;
}


/* pipe_open2:
 *
 *    Open a bi-directional pipe to new process specified by
 *    "command_line".  Forks. Child will exec "command_line", via
 *    "/bin/sh -c". When parent writes to pipe_fileno(ph), the new
 *    child will read that on its stdin. 
 *
 *    The child's stdout will be tied to a file descriptor held by the
 *    parent.  Anything the new child writes to stdout will appear on
 *    that file handle held by the parent. The parent will need to
 *    periodically read from that file handle -- otherwise the child
 *    eventually blocks on write and will probably stop reading its
 *    "stdin" until its output buffer is cleared.
 *
 *    Paramater "command_line" is copied internally -- callers do not
 *    need to worry about keeping the string in memory.
 *
 *    Will update the pipe handle "ph" with new state
 *    information. Returns 0 on success, non-zero on error.
 */
int
pipe_open2 (struct Pipe_Handle *ph, const char *command_line)
{
  // Expect handle initialised and pipe not opened yet.
  assert (ph != NULL);
  assert (PIPES_CLOSED == ph->state);

  // Copy this for later. 
  strncpy (ph->command_line, command_line, ARG_MAX);

  // Create first pipe channel. Parent will WRITE data to child on these. 
  if (pipe (ph->pipe_fds) == -1)
  {
    perror ("Unable to create pipe");
    return -1;
  }

  // Create second pipe channel. Parent will READ data from child on these.
  if (pipe (ph->bipipe_fds) == -1)
  {
    perror ("Unable to create second pipe");
    return -1;
  }


  /* Now we have 2 unidirectional pipes set up:
   *    pipe_fds -- is for parent to send data to child
   *    bipipe_fds -- is for parent to receive data from child
   *
   * So we we want is for parent to write to pipe_fds[1] and the
   * child will read this on its version of pipe_fds[0].
   *
   * Simiarly, the parent will read data from pipe_fds[0] and the
   * child will write data on its version of pipe_fds[1].
   *
   * So always: index [1] is for writing; [0] is for reading.
   */
  ph->state = PIPES_OPENING;


  // Fork here. 
  ph->child_pid = fork ();
  if (-1 == ph->child_pid)
  {
    perror ("Unable to fork");
    return -1;
  }


  /* Parent will continue normally. Child will set up file handles and
   * exec.
   */

  if (0 == ph->child_pid)
  {
    // Am child. I receive data, so don't need the write end of parent's pipe,
    // or read end of bipipe.
    close (ph->pipe_fds[1]);
    close (ph->bipipe_fds[0]);


    /* Child inherits descriptors, which point at the same objects. We
     * want the child's stdin however to be the fd that the parent is
     * writing to, and child's stdout to point to fd parent is reading from.
     */
    if (dup2 (ph->pipe_fds[0], STDIN_FILENO) != STDIN_FILENO)
    {
      perror ("Could not dup2 pipe to stdin");
      abort ();			// TODO: ibid
    }
    if (dup2 (ph->bipipe_fds[1], STDOUT_FILENO) != STDOUT_FILENO)
    {
      perror ("Could not dup2 pipe to stdout");
      abort ();
    }

    // Exec here
    ulog (LOG_INFO, "Child has set up its end. About to exec: %s -c %s",
	  _PATH_BSHELL, ph->command_line);
    if (execl (_PATH_BSHELL, _PATH_BSHELL, "-c", ph->command_line, NULL) ==
	-1)
    {
      ulog (LOG_ERR, "Child was not able to exec: %s -c %s", _PATH_BSHELL,
	    ph->command_line);
      perror ("Error in execl");
      abort ();			// TODO: ibid
    }

    // Child should never get here!
    perror ("Unreachable code reached!");
    abort ();
  }
  else
  {
    // Am parent. I send data, so don't need read end of pipe1 or write end of pipe2.
    close (ph->pipe_fds[0]);
    close (ph->bipipe_fds[1]);
    ulog (LOG_INFO, "Parent has set up its end. Child is pid=%d. Pipe fd=%d ",
	  ph->child_pid, ph->pipe_fds[1]);
  }

  ph->state = PIPES_OPEN_BI;
  return 0;
}


/* pipe_send_eof:
 *
 *    Closes the "write" end of the pipe, which will signal the EOF
 *    condition to the child (piped command). Writes after this will
 *    cause the parent/caller to receive a SIGPIPE signal.
 *
 *    After calling this, parent is expected to read from
 *    "pipe_read_fileno" until EOF. Then call pipe_closed.
 *
 */
void
pipe_send_eof (struct Pipe_Handle *ph)
{
  assert (ph != NULL);
  assert (PIPES_OPEN_BI == ph->state);

  close (pipe_write_fileno (ph));

  ph->state = PIPES_WIDOWED;
  return;
}


/* pipe_close:
 *
 *    Closes the pipe(s) which is expected to terminate the piped
 *    process. Closes both uni and bi-directional pipes. Returns 0 on
 *    success, non-zero on error.
 */
int
pipe_close (struct Pipe_Handle *ph)
{
  int rc = 0;

  assert (ph != NULL);
  assert (ph->state > PIPES_CLOSED);

  // Close pipe handle (parent already closes pipe_fds[0] above)
  close (ph->pipe_fds[1]);

  // Bi-directional pipes need second pipe closed as well
  if (PIPES_OPEN_BI == ph->state)
    close (ph->bipipe_fds[0]);

  ulog_debug ("Parent waiting for child %d to exit (waitpid)", ph->child_pid);

  // Child just got an EOF. Wait for child process (pipe) to terminate
  int stat_loc = 0;
  pid_t reaped_pid = waitpid (ph->child_pid, &stat_loc, 0);
  ulog_debug
    ("waitpid(%d) returned %d. (exited=%d; signalled=%d; stopped=%d)",
     ph->child_pid, reaped_pid, WIFEXITED (stat_loc), WIFSIGNALED (stat_loc),
     WIFSTOPPED (stat_loc));

  // Log what happened to child
  if (reaped_pid == -1)
  {
    ulog (LOG_ERR, "Child process %d could not be reaped (%m)",
	  ph->child_pid);
    perror ("Error returned by waitpid");
  }
  else if (WIFEXITED (stat_loc))
  {
    ulog (LOG_INFO, "Child process %d exited normally with status %d",
	  ph->child_pid, WEXITSTATUS (stat_loc));
  }
  else if (WIFSIGNALED (stat_loc))
  {
    ulog (LOG_INFO, "Child process %d terminated by signal %d", ph->child_pid,
	  WTERMSIG (stat_loc));
  }
  else if (WIFSTOPPED (stat_loc))
  {
    ulog (LOG_INFO,
	  "Child process %d stopped by signal %d -- could be restarted",
	  ph->child_pid, WSTOPSIG (stat_loc));
    // TODO: terminate child process with signal? 
    rc = -1;
  }

  ph->state = PIPES_CLOSED;
  return rc;
}


/* pipe_write_fileno:
 *
 *    Returns the file handle number for the pipe's "stdin" which the
 *    caller wants to write to (so that the child can read it).
 */
int
pipe_write_fileno (struct Pipe_Handle *ph)
{
  assert (ph != NULL);
  assert ((ph->state == PIPES_OPEN_UNI) || (ph->state == PIPES_OPEN_BI));
  return ph->pipe_fds[1];
}

int
pipe_read_fileno (struct Pipe_Handle *ph)
{
  assert (ph != NULL);
  assert ((ph->state == PIPES_OPEN_BI) || (ph->state == PIPES_WIDOWED));
  return ph->bipipe_fds[0];
}


/* pipe_reset:
 *
 *    Closes the pipe, which is expected to cause the child to
 *    terminate and flush all its output buffers. Then re-opens
 *    the pipe. Only works for unidirectional pipes.
 */
int
pipe_reset (struct Pipe_Handle *ph)
{
  int rc = 0;
  assert (ph != NULL);
  assert (ph->state > PIPES_CLOSED);

  // First remember if this was a uni or bi-directional pipe
  int old_state = ph->state;

  // Close pipe(s)
  rc = pipe_close (ph);
  if (rc != 0)
  {
    ulog (LOG_ERR, "Was not able to reset pipe because pipe close failed");
    return rc;
  }

  // Open pipe(s)
  if (PIPES_OPEN_UNI == old_state)
    rc = pipe_open (ph, ph->command_line);
  else if ((PIPES_OPEN_BI == old_state) || (PIPES_WIDOWED == old_state))
    rc = pipe_open2 (ph, ph->command_line);
  else
    rc = -1;

  if (rc != 0)
  {
    ulog (LOG_ERR, "Was not able to reset pipe because could not reopen: %s",
	  ph->command_line);
    return rc;
  }

  ulog_debug ("Reset pipe command: %s", ph->command_line);

  return rc;
}
