/*
 * pipeif.c
 *
 */

#include <sys/types.h>

#include <getopt.h>
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
  int    http_message_complete;
  int    pipe_parts;
};

int cb_header_field(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;
  write(pset->fd_out, at, length);
  write(pset->fd_out, ": ", 2);  
  return 0;
}

int cb_header_value(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;
  write(pset->fd_out, at, length);
  write(pset->fd_out, "\n", 1);
  return 0;
}

int cb_headers_complete(http_parser *parser)
{
  fprintf(stdout, "\n");
  return 0;
}

int cb_body(http_parser *parser, const char *at, size_t length)
{
  struct pipe_settings *pset = (struct pipe_settings*)parser->data;
  write(pset->fd_out, at, length);
  return 0;
}



/*
 * pipe_http_messages:
 *
 * Want to read from stdin ---> send it to pipe's stdin.
 * Read from pipe's stdout ---> send it to my stdout.
 */
int pipe_http_messages(const int pipe_parts, int fd_in, int fd_out)
{
  int errors = 0;
  int rc = EX_OK;

  struct pipe_settings pset;
  pset.fd_in  = fd_in;
  pset.fd_out = fd_out;
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

  /*
   * Want to read from stdin ---> send it to pipe's stdin.
   * Read from pipe's stdout ---> send it to my stdout.
   */
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
	ulog(LOG_ERR, "%s(%d): parser error reading HTTP stream", __FILE__, __LINE__);
      }
    }

  } while ( (bytes_read > 0) && (errors <= 0) );

  free(buffer);

  if ( errors > 0 )
    rc = EX_IOERR;

  return rc;
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

  ulog_init(argv[0]);
  ulog(LOG_INFO, "%s(%d): will pipe %s/%s through %s", __FILE__, __LINE__,
       (piped_parts & PIPE_HEADER ? "headers" : "no headers"),
       (piped_parts & PIPE_BODY ? "body" : "no body"),
       cmd);

  
  rc = pipe_http_messages(piped_parts, STDIN_FILENO, STDOUT_FILENO);

  ulog_close();

  return rc;
}
