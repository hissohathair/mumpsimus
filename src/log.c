/*
 * log.c -- print out features of a HTTP stream on stderr.
 *
 * Not working yet. :)
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "http_parser.h"

/*
 * cb_log_headers_complete: Callback from http_parser, called when all
 * headers have been processed. A good time to extract some relevant
 * log information.
 *
 * Holding some global variables, which is disgusting. Not sure how I
 * can live with myself. I guess I'll struggle on, through the tears,
 * until I eventually die of shame.
 */
static int __http_headers_complete = 0;
static char *__url = NULL;

int cb_log_headers_complete(http_parser *parser) {

  if ( 0 == parser->type ) {
    fprintf(stderr, "%s: [req] %s %s HTTP/%d.%d (%d bytes)\n", __FILE__,
	    http_method_str(parser->method), 
	    (__url == NULL ? "unknown" : __url), 
	    parser->http_major, 
	    parser->http_minor, 
	    parser->nread);
  }
  else {
    fprintf(stderr, "%s: [res] HTTP/%d.%d %d (%d bytes)\n", __FILE__, 
	    parser->http_major, 
	    parser->http_minor, 
	    parser->status_code,
 	    parser->nread);
  }	    

  __http_headers_complete = 1;
  return 0;
}

/*
 * cb_log_url: Callback from http_parser, called when URL
 * read. Preserves the URL for later.
 */
int cb_log_url(http_parser *parser, const char *at, size_t length) {
  strncpy(__url, at, length);
  __url[length] = '\0';
  return 0;
}


/* 
 * pass_http_header: Reading from fd_in, echo bytes to fd_out
 * and print "interesting" messages about the HTTP header
 * being consumed.
 */
int pass_http_header(int fd_in, int fd_out)
{
  int rc = 0;

  char *buf = malloc(sizeof(char) * SSIZE_MAX);
  if ( buf == NULL ) {
    perror("Error from malloc");
    abort();
  }

  http_parser_settings settings;
  init_http_parser_settings(&settings);
  settings.on_url              = cb_log_url;
  settings.on_headers_complete = cb_log_headers_complete;


  http_parser *parser = malloc(sizeof(http_parser));
  if ( NULL == parser ) {
    perror("Error from malloc");
    abort();
  }
  http_parser_init(parser, HTTP_BOTH);

  ssize_t br = 0;
  do {
    memset(buf, 0, SSIZE_MAX);
    br = read(fd_in, buf, SSIZE_MAX);

    size_t nparsed = 0;
    if ( br >= 0 ) {
      write_all(fd_out, buf, br);
      nparsed = http_parser_execute(parser, &settings, buf, br);
      if (nparsed != br) {
	rc = EX_IOERR;
	br = -1;
      }
    }
    else if ( br < 0 ) {
      perror("Error reading data");
      rc = EX_IOERR;
    }
  } while ( (br > 0) && (0 == __http_headers_complete) );
  
  free(buf);
  free(parser);
  return rc;
}


int main(int argc, char *argv[])
{
  __url = malloc(sizeof(char) * SSIZE_MAX);
  if ( NULL == __url ) {
    perror("Error from malloc");
    return EX_OSERR;
  }

  int rc = pass_http_header(STDIN_FILENO, STDOUT_FILENO);
  free(__url);
  return rc;
}
