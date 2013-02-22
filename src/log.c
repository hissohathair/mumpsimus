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

#include "ulog.h"
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

  char *str = malloc(sizeof(char) * SSIZE_MAX);
  if ( NULL == str ) {
    perror("Unable to malloc for logging");
    return 0;
  }

  /* builds the correct log message */
  if ( 0 == parser->type ) {
    snprintf(str, SSIZE_MAX, "[req] %s %s HTTP/%d.%d (%d bytes)",
	     http_method_str(parser->method), 
	     (__url == NULL ? "unknown" : __url), 
	     parser->http_major, 
	     parser->http_minor, 
	     parser->nread);
  }
  else {
    snprintf(str, SSIZE_MAX, "[res] HTTP/%d.%d %d (%d bytes)",
	     parser->http_major, 
	     parser->http_minor, 
	     parser->status_code,
	     parser->nread);
  }	    
  __http_message_complete = 1;

  fprintf(stderr, "%s: %s\n", __FILE__, str);
  ulog(LOG_INFO, "%s", str);

  free(str);

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
    ulog(LOG_DEBUG, "Read %zd bytes from fd=%d", bytes_read, fd_in);

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
      ulog(LOG_DEBUG, "Wrote %zd bytes to fd=%d", bytes_read, fd_out);

      /*
       * Now need to parse http messages. There may be multiple (and even partial) http
       * messages in this stream of bytes. So we repeatedly call parse_http_message until
       * the bytes processed equals the bytes just read into the buffer.
       */
      ssize_t total_parsed = 0;
      ssize_t last_parsed  = 0;
      int max_loops = 5;
      do {
	ulog(LOG_DEBUG, "About to parse from offset %zd, where char=%c (%d)\n",
	     total_parsed, buffer[total_parsed], buffer[total_parsed]);

	last_parsed = http_parser_execute(parser, &settings, buffer + total_parsed, bytes_read - total_parsed);

	ulog(LOG_DEBUG, "Parsed %zd bytes from %zd (total now %zd; %zd remains)\n",
	     last_parsed, bytes_read, 
	     total_parsed + last_parsed,
	     bytes_read - total_parsed - last_parsed);

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
	  ulog(LOG_DEBUG, "Complete message detected. Resetting parser.");
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
  ulog_init(argv[0]);

  __url = malloc(sizeof(char) * SSIZE_MAX);
  if ( NULL == __url ) {
    perror("Error from malloc");
    return EX_OSERR;
  }

  int rc = pass_http_messages(STDIN_FILENO, STDOUT_FILENO);

  free(__url);
  ulog_close();

  return rc;
}
