/*
 * pipeif.c -- pipe part of a HTTP message if it matches certain
 * criteria.
 *
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <paths.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "http_parser.h"
#include "util.h"
#include "ulog.h"
#include "stream_buffer.h"


#define PIPE_HEADER 0x01
#define PIPE_BODY 0x02
#define SEM_NAME "pipeif-semaphore"

struct pipe_settings {
  int    fd_in;
  int    fd_out;
  int    fd_pipe;
  int    http_message_complete;
  int    pipe_parts;
  sem_t *header_sem;
  char  *url;
  struct Stream_Buffer *sbuf;
};


/*
 * cb_url: Callback from http_parser, called when URL read. Preserves
 * the URL for later.
 */
int cb_url(http_parser *parser, const char *at, size_t length) 
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;

  if ( length < URL_MAX ) {
    strncpy(pset->url, at, length);
    pset->url[length] = '\0';
  }
  else {
    strncpy(pset->url, at, URL_MAX-1);
    pset->url[URL_MAX-1] = '\0';
    ulog(LOG_ERR, "URL exceeded %zd bytes and was truncated", URL_MAX);
  }
  ulog_debug("Recorded URL of HTTP message: %s", pset->url);
  return 0;
}


int cb_headers_complete(http_parser *parser)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;

  char *str = malloc(sizeof(char) * BUFFER_MAX);
  if ( NULL == str ) {
    perror("Error in malloc");
    return -1;
  }

  /* builds the correct log message */
  if ( 0 == parser->type ) {
    snprintf(str, BUFFER_MAX, "%s %s HTTP/%d.%d\r\n",
	     http_method_str(parser->method), 
	     pset->url,
	     parser->http_major, 
	     parser->http_minor);
  }
  else {
    snprintf(str, BUFFER_MAX, "HTTP/%d.%d %s\r\n",
	     parser->http_major, 
	     parser->http_minor, 
	     pset->url);
  }

  int fd = pset->fd_out;
  if ( pset->pipe_parts & PIPE_HEADER )
    fd = pset->fd_pipe;
  ulog_debug("cb_headers_complete(parser=%X) -> fd=%d", parser, fd);

  write_all(fd, str, strlen(str));
  stream_buffer(pset->sbuf, "\r\n", 2);
  stream_buffer_write(pset->sbuf, fd);
  //usleep(5000);

  if ( pset->header_sem != NULL ) {
    int r = sem_post(pset->header_sem);
    if ( r != 0 ) {
      perror("Could not post to semaphore to say body is no OK to write");
      ulog(LOG_ERR, "Error trying to sem_post (%s). This might get ugly.", strerror(errno));
    }
  }

  free(str);

  return 0;
}

int cb_status_complete(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;

  if ( length < URL_MAX ) {
    strncpy(pset->url, at, length);
    pset->url[length] = '\0';
  }
  else {
    strncpy(pset->url, at, URL_MAX-1);
    pset->url[URL_MAX] = '\0';
  }
  ulog_debug("HTTP response status: %s", pset->url);
  return 0;
}


int cb_header_field(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;
  stream_buffer(pset->sbuf, at, length);
  stream_buffer(pset->sbuf, ": ", 2);  
  return 0;
}

int cb_header_value(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;
  stream_buffer(pset->sbuf, at, length);
  stream_buffer(pset->sbuf, "\r\n", 2);
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
int pipe_http_messages(const int pipe_parts, int fd_in, int fd_out, int fd_pipe, sem_t *header_sem)
{
  int errors = 0;
  int rc = EX_OK;

  struct pipe_settings pset;
  pset.fd_in = fd_in;
  pset.fd_out = fd_out;
  pset.fd_pipe = fd_pipe;
  pset.pipe_parts = pipe_parts;
  pset.http_message_complete = 0;
  pset.header_sem = header_sem;
  pset.url = malloc(sizeof(char) * URL_MAX);
  pset.sbuf = stream_buffer_new();
  if ( NULL == pset.url || NULL == pset.sbuf ) {
    perror("Error in malloc or stream buffer");
    return -1;
  }
  pset.url[0] = '\0';

  http_parser_settings settings;
  init_http_parser_settings(&settings);
  settings.on_header_field = cb_header_field;
  settings.on_header_value = cb_header_value;
  settings.on_headers_complete = cb_headers_complete;
  settings.on_status_complete = cb_status_complete;
  settings.on_url = cb_url;
  settings.on_body = cb_body;

  http_parser parser;
  http_parser_init(&parser, HTTP_BOTH);
  parser.data = (void*)&pset;


  char *buffer = malloc(sizeof(char) * BUFFER_MAX);
  if ( NULL == buffer ) {
    perror("Error from malloc");
    return -1;
  }


  ssize_t last_parsed = 0;
  ssize_t bytes_read  = 0;
  char *buf_ptr = buffer;
  do {
    memset(buffer, 0, BUFFER_MAX);
    buf_ptr = buffer;
    bytes_read = read(fd_in, buffer, BUFFER_MAX);
    ulog_debug("Read %zd bytes from fd=%d", bytes_read, fd_in);

    while ( (bytes_read >= 0) && (errors <= 0) ) {
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
  free(pset.url); pset.url = NULL;
  stream_buffer_delete(pset.sbuf);

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
    //qclose(STDIN_FILENO);
    if ( dup2(fds[0], STDIN_FILENO) != STDIN_FILENO ) {
      perror("Could not dup2 pipe to stdin");
      abort();
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
  fprintf(stderr, "usage: %s [-hbl] -c command\n", ident);
  exit( EX_USAGE );
}


/* 
 * main
 */
int main(int argc, char *argv[])
{
  int rc = EX_OK;
  int piped_parts = 0;
  int use_semaphore = 0;
  char *cmd = NULL;

  opterr = 0;
  int c = 0;
  while ( (c = getopt(argc, argv, "hblc:")) != -1 ) {
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
      case 'l':
	use_semaphore = 1;
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
  if ( use_semaphore && !(piped_parts & PIPE_BODY) ) {
    use_semaphore = 0;
  }

  sem_t *header_sem = NULL;
  if ( use_semaphore ) {
    (void)sem_unlink(SEM_NAME);
    header_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRWXU, 0);
    if ( SEM_FAILED == header_sem ) {
      perror("Unable to create semaphore for locking");
      return EX_OSERR;
    }
  } 

  ulog_init(argv[0]);
  ulog(LOG_INFO, "Will pipe %s/%s through [%s] with%s",
       (piped_parts & PIPE_HEADER ? "headers" : "no headers"),
       (piped_parts & PIPE_BODY ? "body" : "no body"),
       cmd, (use_semaphore ? " locking" : "out locking"));

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
    ulog(LOG_INFO, "%s: Reading fd=%d, unfiltered out to fd=%d, filtered out to fd=%d, sem=%d", argv[0],
	 STDIN_FILENO, STDOUT_FILENO, pipe_fds[1], header_sem);
    rc = pipe_http_messages(piped_parts, STDIN_FILENO, STDOUT_FILENO, pipe_fds[1], header_sem);
    close_pipe(pipe_fds, pipe_pid);
  }

  ulog_close();
  sem_close(header_sem);
  sem_unlink(SEM_NAME);

  return rc;
}
