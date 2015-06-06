
#include <assert.h>
#include <paths.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "pipes.h"
#include "ulog.h"


/* pipe_handle_init:
 *
 *    Initialise the pipe handle structure.
 */
void pipe_handle_init(struct Pipe_Handle *ph)
{
  ph->pipe_status = 0; // closed
  ph->pipe_fds[0] = STDIN_FILENO;
  ph->pipe_fds[1] = STDERR_FILENO; // TODO: why?
  ph->pipe_pid = 0;
  ph->pipe_cmd = NULL;
  return;
}


/* pipe_open: 
 *
 *    Forks. Child will exec "pipe_cmd", via "/bin/sh -c". Parameter
 *    "fd_in" will hold the read end of the pipe; "fd_out" the write
 *    end. Parameter "pid" is the process ID of the child process (ie
 *    the one that is now running the piped command).
 */
int pipe_open(struct Pipe_Handle *ph, const char *pipe_cmd) 
{
  // Expect handle initialised and pipe not opened yet.
  assert(ph->pipe_status == 0);

  // Copy this for later.
  ph->pipe_cmd = pipe_cmd; // TODO: Copy for real or just pointer?

  // Create pipe channel. Parent will send data to child. Child will send data to same stdout as parent.
  if ( pipe(ph->pipe_fds) == -1 ) {
    perror("Unable to create pipe");
    return -1;
  }


  /* Now we have a unidirectional pipe set up in pipe_fds. Data
   * written to pipe_fds[1] appears on (ie can be read from)
   * pipe_fds[0].
   *
   * So we we want is for parent to write to pipe_fds[1] and the
   * child will read this on its version of pipe_fds[0].
   */
  

  // Fork here. 
  ph->pipe_pid = fork();
  if ( -1 == ph->pipe_pid ) {
    perror("Unable to fork");
    return -1;
  }

  /* Parent will continue normally. Child will set up file handles and
   * exec.
   */

  if ( 0 == ph->pipe_pid ) {
    // Am child. I receive data, so don't need the write end of pipe. 
    close(ph->pipe_fds[1]);

    /* Child inherits descriptors, which point at the same objects. We
     * want the child's stdin however to be the fd that the parent is
     * writing to. Happy to leave stdout pointing at stdout, but would
     * prefer that unbuffered since we're going to have a hell of a
     * time keeping this output sane otherwise.
     */
    if ( dup2(ph->pipe_fds[0], STDIN_FILENO) != STDIN_FILENO ) {
      perror("Could not dup2 pipe to stdin");
      abort();
    }

    // Exec here
    ulog(LOG_INFO, "Child has set up its end. About to exec: %s -c %s", _PATH_BSHELL, ph->pipe_cmd);
    if ( execl(_PATH_BSHELL, _PATH_BSHELL, "-c", ph->pipe_cmd, NULL) == -1 ) {
      ulog(LOG_ERR, "Child was not able to exec: %s -c %s", _PATH_BSHELL, ph->pipe_cmd);
      perror("Error in execl");
      abort();
    }

    // Child should never get here!
    perror("Unreachable code reached!");
    abort();
  } 
  else {
    // Am parent. I send data, so don't need read end of pipe.
    close(ph->pipe_fds[0]);
    ulog(LOG_INFO, "Parent has set up its end. Child is pid=%d. Pipe fd=%d ", ph->pipe_pid, ph->pipe_fds[1]);
  }

  ph->pipe_status = 1; // open
  return 0;
}



/* pipe_close:
 *
 *    Closes the pipe which is expected to terminate the piped
 *    process.
 */
int pipe_close(struct Pipe_Handle *ph)
{
  assert(ph->pipe_status == 1);

  // Close pipe handles 
  close(ph->pipe_fds[0]); // TODO: was already closed though?
  close(ph->pipe_fds[1]);

  // Wait for child process (pipe) to terminate
  ulog_debug("Parent waiting for child %d to exit (waitpid)", ph->pipe_pid);
  int stat_loc = 0;
  pid_t reaped_pid = waitpid(ph->pipe_pid, &stat_loc, 0);
  ulog_debug("waitpid(%d) returned %d. (exited=%d; signalled=%d; stopped=%d)", ph->pipe_pid, reaped_pid,
	     WIFEXITED(stat_loc), WIFSIGNALED(stat_loc), WIFSTOPPED(stat_loc));

  // Log what happened to child
  if ( reaped_pid == -1 ) {
    ulog(LOG_ERR, "Child process %d could not be reaped (%m)", ph->pipe_pid);
    perror("Error returned by waitpid");
  }
  else if ( WIFEXITED(stat_loc) ) {
    ulog(LOG_INFO, "Child process %d exited normally with status %d", ph->pipe_pid, WEXITSTATUS(stat_loc));
  }
  else if ( WIFSIGNALED(stat_loc) ) {
    ulog(LOG_INFO, "Child process %d terminated by signal %d", ph->pipe_pid, WTERMSIG(stat_loc));
  }
  else if ( WIFSTOPPED(stat_loc) ) {
    ulog(LOG_INFO, "Child process %d stopped by signal %d -- could be restarted", ph->pipe_pid, WSTOPSIG(stat_loc));
    // TODO: terminate child process with signal? 
  }

  ph->pipe_status = 0;
  return 0;
}


/* pipe_stdout:
 *
 *    Returns the file handle number for the pipe's stdout.
 */
int pipe_fileno(struct Pipe_Handle *ph)
{
  assert(ph->pipe_status == 1);
  return ph->pipe_fds[1];
}


/* pipe_reset:
 *
 *    Closes the pipe, which is expected to cause the child to
 *    terminate and flush all its output buffers. Then re-opens
 *    the pipe.
 */
int pipe_reset(struct Pipe_Handle *ph)
{
  int rc = 0;
  assert(ph->pipe_status == 1);


  rc = pipe_close(ph);
  if ( rc != 0 ) {
    ulog(LOG_ERR, "Was not able to reset pipe because pipe close failed");
    return rc;
  }

  rc = pipe_open(ph, ph->pipe_cmd);
  if ( rc != 0 ) {
    ulog(LOG_ERR, "Was not able to reset pipe because could not reopen: %s", ph->pipe_cmd);
    return rc;
  }

  ulog_debug("Reset pipe command: %s", ph->pipe_cmd);

  return rc;

}
