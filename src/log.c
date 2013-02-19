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
 * cb_log_message_complete: Callback from http_parser, called when all
 * headers have been processed. A good time to extract some relevant
 * log information.
 *
 * Holding some global variables, which is disgusting. Not sure how I
 * can live with myself. I guess I'll struggle on, through the tears,
 * until I eventually die of shame.
 */
static char *__url = NULL;
static int __http_message_complete = 0;
int cb_log_message_complete(http_parser *parser) {

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
  __http_message_complete = 1;
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
 * pass_http_messages: Reading from fd_in, echo bytes to fd_out
 * and print "interesting" messages about the HTTP header
 * being consumed.
 */
int pass_http_messages(int fd_in, int fd_out)
{
  int rc = 0;
  int errors = 0;

  http_parser_settings settings;
  init_http_parser_settings(&settings);
  settings.on_url              = cb_log_url;
  settings.on_message_complete = cb_log_message_complete;

  http_parser *parser = malloc(sizeof(http_parser));
  if ( NULL == parser ) {
    perror("Error from malloc");
    abort();
  }
  http_parser_init(parser, HTTP_BOTH);

  char *buffer = malloc(sizeof(char) * SSIZE_MAX);
  if ( buffer == NULL ) {
    perror("Error from malloc");
    abort();
  }

  ssize_t bytes_read = 0;
  do {
    /* Reading from source until eof */
    memset(buffer, 0, SSIZE_MAX);
    bytes_read = read(fd_in, buffer, SSIZE_MAX);
    //fprintf(stderr, "%s(%d): Read %d bytes from %d\n", __FILE__, __LINE__, bytes_read, fd_in);

    /* br==0 means eof which has to be processed by the parser just like data read */
    if ( 0 == bytes_read ) {
      ssize_t last_parsed = http_parser_execute(parser, &settings, buffer, 0);
      if ( last_parsed < 0 ) {
	perror("Error reading HTTP message stream");
	errors++;
	rc = EX_IOERR;
      }
    }
    else if ( bytes_read > 0 ) {

      /* No matter what we always echo what we've read */
      write_all(fd_out, buffer, bytes_read);
      //fprintf(stderr, "%s(%d): Wrote %d bytes to %d\n", __FILE__, __LINE__, bytes_read, fd_out);

      /*
       * Now need to parse http messages. There may be multiple (and even partial) http
       * messages in this stream of bytes. So we repeatedly call parse_http_message until
       * the bytes processed equals the bytes just read into the buffer.
       */
      ssize_t total_parsed = 0;
      ssize_t last_parsed  = 0;
      int max_loops = 5;
      do {
	/*	fprintf(stderr, "%s(%d): About to parse from offset %d, where char=%c (%d)\n", __FILE__, __LINE__,
		total_parsed, buffer[total_parsed], buffer[total_parsed]);*/
	last_parsed = http_parser_execute(parser, &settings, buffer + total_parsed, bytes_read - total_parsed);
	/*fprintf(stderr, "%s(%d): Parsed %d bytes from %d (total now %d; %d remains)\n", __FILE__, __LINE__, 
		last_parsed, bytes_read, 
		total_parsed + last_parsed,
		bytes_read - total_parsed - last_parsed);*/
	if ( last_parsed < 0 ) {
	  perror("Error reading HTTP message stream");
	  errors++;
	  rc = EX_IOERR;
	}
	else {
	  total_parsed += last_parsed;
	  /* BUG: so this is interesting, http_parser seems to over-consume by one byte in concat messages */
	  if ( total_parsed < bytes_read )
	    total_parsed--;
	}

	/* 
	 * During that last call a complete message may have been read, which set a global
	 * (TODO: remove global). Need to check this and restart the parser if that happened.
	 */
	if ( 1 == __http_message_complete ) {
	  //fprintf(stderr, "%s(%d): Complete message detected. Resetting parser.\n", __FILE__, __LINE__);
	  http_parser_init(parser, HTTP_BOTH);
	  __http_message_complete = 0;
	}

      } while ( (total_parsed < bytes_read) && (last_parsed >= 0) && (errors <= 0) && (--max_loops > 0));
    }
    else if ( bytes_read < 0 ) {
      perror("Error reading data");
      errors++;
      rc = EX_IOERR;
    }
  } while ( (bytes_read > 0) && (errors <= 0) );
  
  free(parser);
  free(buffer);

  return rc;
}


int main(int argc, char *argv[])
{
  __url = malloc(sizeof(char) * SSIZE_MAX);
  if ( NULL == __url ) {
    perror("Error from malloc");
    return EX_OSERR;
  }

  int rc = pass_http_messages(STDIN_FILENO, STDOUT_FILENO);
  free(__url);
  return rc;
}
