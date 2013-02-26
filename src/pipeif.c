/*
 * pipeif.c
 *
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <getopt.h>
#include <paths.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "http_parser.h"
#include "util.h"
#include "ulog.h"


#define PIPE_HEADER 0x01
#define PIPE_BODY 0x02

struct pipe_settings {
  int    fd_in;
  int    fd_out;
  int    fd_pipe;
  int    http_message_complete;
  int    pipe_parts;
};


int cb_header_field(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;

  int fd = pset->fd_out;
  if ( pset->pipe_parts & PIPE_HEADER )
    fd = pset->fd_pipe;
  ulog_debug("cb_header_field(parser=%X, at=%X, length=%zd) -> fd=%d", parser, at, length, fd);

  write_all(fd, at, length);
  write_all(fd, ": ", 2);  
  return 0;
}

int cb_header_value(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;

  int fd = pset->fd_out;
  if ( pset->pipe_parts & PIPE_HEADER )
    fd = pset->fd_pipe;
  ulog_debug("cb_header_value(parser=%X, at=%X, length=%zd) -> fd=%d", parser, at, length, fd);

  write_all(fd, at, length);
  write_all(fd, "\n", 1);
  return 0;
}

int cb_headers_complete(http_parser *parser)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;

  int fd = pset->fd_out;
  if ( pset->pipe_parts & PIPE_HEADER )
    fd = pset->fd_pipe;
  ulog_debug("cb_headers_complete(parser=%X) -> fd=%d", parser, fd);

  write_all(fd, "\n", 1);

  return 0;
}

int cb_body(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;

  int fd = pset->fd_out;
  if ( pset->pipe_parts & PIPE_BODY )
    fd = pset->fd_pipe;
  ulog_debug("cb_body(parser=%X, at=%X, length=%zd) -> fd=%d", parser, at, length, fd);

  write_all(fd, at, length);

  return 0;
}



/*
 * pipe_http_messages:
 *
 * Want to read from stdin ---> send it to pipe's stdin.
 * Read from pipe's stdout ---> send it to my stdout.
 */
int pipe_http_messages(const int pipe_parts, int fd_in, int fd_out, int fd_pipe)
{
  int errors = 0;
  int rc = EX_OK;

  struct pipe_settings pset;
  pset.fd_in   = fd_in;
  pset.fd_out  = fd_out;
  pset.fd_pipe = fd_pipe;
  pset.pipe_parts = pipe_parts;
  pset.http_message_complete = 0;

  http_parser_settings settings;
  init_http_parser_settings(&settings);
  settings.on_header_field = cb_header_field;
  settings.on_header_value = cb_header_value;
  settings.on_headers_complete = cb_headers_complete;
  settings.on_body = cb_body;

  http_parser parser;
  http_parser_init(&parser, HTTP_BOTH);
  parser.data = (void*)&pset;


  char *buffer = malloc(sizeof(char) * SSIZE_MAX);
  if ( NULL == buffer ) {
    perror("Error from malloc");
    abort();
  }


  ssize_t last_parsed = 0;
  ssize_t bytes_read  = 0;
  char *buf_ptr = buffer;
  do {
    memset(buffer, 0, SSIZE_MAX);
    buf_ptr = buffer;
    bytes_read = read(fd_in, buffer, SSIZE_MAX);
    ulog_debug("Read %zd bytes from fd=%d", bytes_read, fd_in);

    while ( bytes_read >= 0 ) {
      last_parsed = http_parser_execute(&parser, &settings, buf_ptr, bytes_read);
      ulog_debug("Parsed %zd bytes out of %zd bytes remaining", last_parsed, bytes_read);

      if ( 0 == bytes_read ) {
	/* was eof. we've told the parser, so now exit loop */
	bytes_read = -1;
      }
      else if ( last_parsed > 0 ) {
	bytes_read -= last_parsed;
	buf_ptr    += last_parsed - 1;
      }
      else {
	errors++;
	ulog(LOG_ERR, "Parser error reading HTTP stream");
      }
    }
    ulog_debug("Inner parser loop exited (bytes_read=%zd; errors=%d)", bytes_read, errors);

  } while ( (bytes_read > 0) && (errors <= 0) );

  free(buffer);

  if ( errors > 0 )
    rc = EX_IOERR;

  return rc;
}


/* 
 * open_pipe: Forks. Child will exec "cmd", via "/bin/sh -c". Parameter "fd_in" will hold
 * the read end of the pipe; "fd_out" the write end. Parameter "pid" is the process ID of
 * the child process (ie the one that is now running the piped command).
 */
int open_pipe(const char *cmd, int fds[2], pid_t *pid)
{

  /* Create pipe channel. Parent will send data to child. Child will end data to same stdout as parent. */
  if ( pipe(fds) == -1 ) {
    perror("Unable to create pipe");
    return -1;
  }

  /* Parent will continue normally. Child will set up handles and exec. */
  *pid = fork();
  if ( -1 == *pid ) {
    perror("Unable to fork");
    return -1;
  }

  if ( 0 == *pid ) {
    /* Am child. I receive data, so don't need the write end of pipe. */
    close(fds[1]);

    /*
     * Child inherits descriptors, which point at the same objects. We want the
     * child's stdin however to be a fd that we write to selectively. Happy
     * to leave stdout pointing at stdout, but would prefer that unbuffered since
     * we're going to have a hell of a time keeping this output sane otherwise.
     */
    close(STDIN_FILENO);
    if ( dup(fds[0]) != STDIN_FILENO ) {
      perror("Could not dup2 pipe to stdin");
      abort();
    }

    if ( fcntl(STDOUT_FILENO, F_NOCACHE, 1) == -1 ) {
      perror("Error in fcntl when disabling cache");
      /* TODO: Revisit this. Should we still continue or abort? */
    }

    ulog(LOG_INFO, "Child has set up its end. About to exec: %s -c %s", _PATH_BSHELL, cmd);
    if ( execl(_PATH_BSHELL, _PATH_BSHELL, "-c", cmd, NULL) == -1 ) {
      ulog(LOG_ERR, "Child was not able to exec: %s -c %s", _PATH_BSHELL, cmd);
      perror("Error in execl");
      abort();
    }

    /* child should never get here */
    perror("Unreachable code reached!");
    abort();
  } 
  else {
    /* Am parent. I send data, so don't need read end of pipe. */
    close(fds[0]);
    if ( fcntl(STDOUT_FILENO, F_NOCACHE, 1) == -1 ) {
      perror("Error in fcntl when disabling cache");
    }
    ulog(LOG_INFO, "Parent has set up its end. Child is pid=%d. Pipe fd=%d ", *pid, fds[1]);
  }

  if ( *pid < 0 )
    return -1;
  else
    return 0;
}

void close_pipe(int fds[2], pid_t pid)
{
  /* Close pipe handles */
  close(fds[0]); /* was already closed though */
  close(fds[1]);

  ulog_debug("Parent waiting for child %d to exit (waitpid)", pid);
  int stat_loc = 0;
  pid_t reaped_pid = waitpid(pid, &stat_loc, 0);
  ulog_debug("waitpid(%d) returned %d. (exited=%d; signalled=%d; stopped=%d)", pid, reaped_pid,
	     WIFEXITED(stat_loc), WIFSIGNALED(stat_loc), WIFSTOPPED(stat_loc));

  if ( reaped_pid == -1 ) {
    ulog(LOG_ERR, "Child process %d could not be reaped (%m)", pid);
    perror("Error returned by waitpid");
  }
  else if ( WIFEXITED(stat_loc) ) {
    ulog(LOG_INFO, "Child process %d exited normally with status %d", pid, WEXITSTATUS(stat_loc));
  }
  else if ( WIFSIGNALED(stat_loc) ) {
    ulog(LOG_INFO, "Child process %d terminated by signal %d", pid, WTERMSIG(stat_loc));
  }
  else if ( WIFSTOPPED(stat_loc) ) {
    ulog(LOG_INFO, "Child process %d stopped by signal %d -- could be restarted", pid, WSTOPSIG(stat_loc));
    /* TODO: terminate child process with signal? */
  }

  return;
}


/*
 * usage: Command usage and abort.
 */
void usage(const char *ident, const char *message)
{
  if ( message != NULL )
    fprintf(stderr, "error: %s\n", message);
  fprintf(stderr, "usage: %s [-hb] -c command\n", ident);
  exit( EX_USAGE );
}


/* 
 * main
 */
int main(int argc, char *argv[])
{
  int rc = EX_OK;
  int piped_parts = 0;
  char *cmd = NULL;

  opterr = 0;
  int c = 0;
  while ( (c = getopt(argc, argv, "hbc:")) != -1 ) {
    switch ( c )
      {
      case 'h':
	piped_parts |= PIPE_HEADER;
	break;
      case 'b':
	piped_parts |= PIPE_BODY;
	break;
      case 'c':
	if ( NULL == optarg )
	  usage(argv[0], "Must pass a command to -c");
	cmd = optarg;
	break;
      case '?':
	usage(argv[0], "Unknown argument");
	break;
      }
  }

  if ( NULL == cmd ) {
    usage(argv[0], "Must provide a command to pipe through with -c <command>");
  }
  if ( 0 == piped_parts ) {
    usage(argv[0], "Must provide either -h or -b");
  }

  ulog_init(argv[0]);
  ulog(LOG_INFO, "Will pipe %s/%s through %s",
       (piped_parts & PIPE_HEADER ? "headers" : "no headers"),
       (piped_parts & PIPE_BODY ? "body" : "no body"),
       cmd);

  if ( system(NULL) == 0 ) {
    perror("System shell is not available");
    return EX_OSERR;
  }

  int pipe_fds[2] = { STDIN_FILENO, STDERR_FILENO } ;
  pid_t pipe_pid = 0;
  if ( open_pipe(cmd, pipe_fds, &pipe_pid) != 0 ) {
    ulog(LOG_ERR, "%s: Unable to open pipe to: %s", argv[0], cmd);
    rc = EX_UNAVAILABLE;
  }
  else {
    ulog(LOG_INFO, "%s: Reading fd=%d, unfiltered out to fd=%d, filtered out to fd=%d", argv[0],
	 STDIN_FILENO, STDOUT_FILENO, pipe_fds[1]);
    rc = pipe_http_messages(piped_parts, STDIN_FILENO, STDOUT_FILENO, pipe_fds[1]);
    close_pipe(pipe_fds, pipe_pid);
  }

  ulog_close();

  return rc;
}
