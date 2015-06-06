/*
 * log.c -- print out features of a HTTP stream from stdin and report
 * on stderr.
 *
 * Not working yet. :)
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "ulog.h"
#include "util.h"
#include "http_parser.h"

/*
 * Holding some global variables, which is disgusting. Not sure how I
 * can live with myself. I guess I'll struggle on, through the tears,
 * until I eventually die of shame.
 */
static char *__url = NULL;
static char *__headers = NULL;
static int __http_message_complete = 0;
static int __volume = 1; /* 0=quiet; 1=normal; 2=verbose */


/*
 * usage: print usage message and abort.
 */
void usage(const char *ident)
{
  fprintf(stderr, "usage: %s [-qv]\n", ident);
  exit( EX_USAGE );
}


/*
 * log_highlight: Print terminal escape sequence to turn colour on and
 * off.
 */
void log_highlight(FILE *outf, const int turn_highlight_on)
{
  if ( getenv("TERM") == NULL )
    return;
  if ( strstr(getenv("TERM"), "color") == NULL )
    return;

  /* TODO: ummm... there's a termcap library for this... */
  if ( turn_highlight_on )
    fprintf(outf, "%c[1;32m", 27);
  else
    fprintf(outf, "%c[0m", 27);
}


/*
 * cb_log_message_complete: Callback from http_parser, called when all
 * headers have been processed. A good time to extract some relevant
 * log information.
 */
int cb_log_message_complete(http_parser *parser) 
{
  char *str = malloc(sizeof(char) * BUFFER_MAX);
  if ( NULL == str ) {
    perror("Unable to malloc for logging");
    return 0;
  }

  /* builds the correct log message */
  if ( 0 == parser->type ) {
    snprintf(str, BUFFER_MAX, "[req] %s %s HTTP/%d.%d (%d bytes)",
	     http_method_str(parser->method), 
	     ((__url == NULL || !__url[0]) ? "unknown" : __url), 
	     parser->http_major, 
	     parser->http_minor, 
	     parser->nread);
  }
  else {
    snprintf(str, BUFFER_MAX, "[res] HTTP/%d.%d %d (%zd bytes)",
	     parser->http_major, 
	     parser->http_minor, 
	     parser->status_code,
	     (ssize_t)(parser->nread + parser->content_length));
  }

  ulog(LOG_INFO, "cb_log_message_complete: %s", str);
  if ( __volume > 0 ) {
    log_highlight(stderr, 1);
    fprintf(stderr, "%s: %s\n", __FILE__, str);
    log_highlight(stderr, 0);
  }

  if ( __volume > 1 )
    fprintf(stderr, "%s: [begin headers]\n%s%s: [end headers]\n", __FILE__, __headers, __FILE__);

  __http_message_complete = 1;
  __headers[0] = '\0';
  free(str);

  return 0;
}

/*
 * cb_log_url: Callback from http_parser, called when URL
 * read. Preserves the URL for later.
 */
int cb_log_url(http_parser *parser, const char *at, size_t length) 
{
  if ( length < BUFFER_MAX ) {
    strncpy(__url, at, length);
    __url[length] = '\0';
  }
  else {
    strncpy(__url, at, BUFFER_MAX-1);
    __url[BUFFER_MAX-1] = '\0';
    ulog(LOG_ERR, "%s(%d): URL exceeded %zd bytes and was truncated", __FILE__, __LINE__, BUFFER_MAX);
  }
  return 0;
}

/*
 * cb_log_header_field: Called from http_parser, when a header field
 * has been read. Adds this to our own buffer for later logging.
 */
int cb_log_header_field(http_parser *parser, const char *at, size_t length) 
{
  char *new_field = strndup(at, length);
  strlcat(__headers, "\t", BUFFER_MAX);
  strlcat(__headers, new_field, BUFFER_MAX);
  strlcat(__headers, ": ", BUFFER_MAX);
  free(new_field);
  return 0;
}

/*
 * cb_log_header_value: Called from http_parser, when a header field's
 * value has been read. Adds this to our own buffer for later logging.
 */
int cb_log_header_value(http_parser *parser, const char *at, size_t length) 
{
  char *new_value = strndup(at, length);
  strlcat(__headers, new_value, BUFFER_MAX);
  strlcat(__headers, "\n", BUFFER_MAX);
  free(new_value);
  return 0;
}

int cb_log_body(http_parser *parser, const char *at, size_t length)
{
  ulog_debug("cb_log_body called (len=%zd)", length);
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
  http_parser_settings_init(&settings);
  settings.on_url              = cb_log_url;
  settings.on_message_complete = cb_log_message_complete;
  settings.on_body             = cb_log_body;
  if ( __volume > 1 ) {
    settings.on_header_field   = cb_log_header_field;
    settings.on_header_value   = cb_log_header_value;
  }

  http_parser *parser = malloc(sizeof(http_parser));
  if ( NULL == parser ) {
    perror("Error from malloc");
    abort();
  }
  http_parser_init(parser, HTTP_BOTH);

  char *buffer = malloc(sizeof(char) * BUFFER_MAX);
  if ( buffer == NULL ) {
    perror("Error from malloc");
    abort();
  }

  ssize_t bytes_read = 0;
  do {
    /* Reading from source until eof */
    memset(buffer, 0, BUFFER_MAX);
    bytes_read = read(fd_in, buffer, BUFFER_MAX);
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
      do {
	ulog(LOG_DEBUG, "About to parse from offset %zd, where next char's are =(%d,%d,%d = %c%c%c)\n", 
	     total_parsed, 
	     buffer[total_parsed], buffer[total_parsed+1], buffer[total_parsed+2], 
	     buffer[total_parsed], buffer[total_parsed+1], buffer[total_parsed+2]);

	last_parsed = http_parser_execute(parser, &settings, buffer + total_parsed, bytes_read - total_parsed);

	ulog(LOG_DEBUG, "Parsed %zd bytes from %zd (total now %zd; %zd remains; target = %zd)\n",
	     last_parsed, bytes_read, 
	     total_parsed + last_parsed,
	     bytes_read - total_parsed - last_parsed,
	     (total_parsed + last_parsed) + (bytes_read - total_parsed - last_parsed));

	if ( last_parsed < 0 ) {
	  perror("Error reading HTTP message stream");
	  errors++;
	  rc = EX_IOERR;
	}
	else {
	  total_parsed += last_parsed;
	  /* 
	   * BUG: so this is interesting, http_parser seems to over-consume by one byte in concat messages.
	   * See http_parser.c, tag [HISSO].
	   */
	  ulog_debug("Adjustment check: total_parsed now = %zd; bytes_read = %zd; bheb=%d; next char = %d (%c)", 
		     total_parsed, bytes_read, parser->body_had_extra_byte,
		     buffer[total_parsed], buffer[total_parsed]);
	  if ( (total_parsed < bytes_read) && (parser->body_had_extra_byte == 0) )
	    total_parsed--;
	  ulog_debug("Adjustment now: total_parsed now = %zd; bytes_read = %zd; bheb=%d; next char = %d (%c)", 
		     total_parsed, bytes_read, parser->body_had_extra_byte,
		     buffer[total_parsed], buffer[total_parsed]);
	}

	/* 
	 * During that last call a complete message may have been read, which set a global
	 * (TODO: remove global). Need to check this and restart the parser if that happened.
	 */
	if ( 1 == __http_message_complete ) {
	  ulog(LOG_DEBUG, "Complete message detected. Resetting parser.");
	  http_parser_init(parser, HTTP_BOTH);
	  parser->nread = 0;
	  parser->content_length = 0;
	  __http_message_complete = 0;
	}

      } while ( (total_parsed < bytes_read) && (last_parsed >= 0) && (errors <= 0) );
      ulog_debug("Exit state: total_parsed=%zd; bytes_read=%zd; last_parsed=%zd; errors=%d.",
		 total_parsed, bytes_read, last_parsed, errors);
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


/*
 * main: Check command line arguments, then beging processing.
 */
int main(int argc, char *argv[])
{
  __url     = malloc(sizeof(char) * BUFFER_MAX);
  __headers = malloc(sizeof(char) * BUFFER_MAX);
  if ( (NULL == __url) || (NULL == __headers) ) {
    perror("Error from malloc");
    return EX_OSERR;
  }
  __url[0] = '\0';
  __headers[0] = '\0';

  opterr = 0;
  int c = 0;
  while ( (c = getopt(argc, argv, "qv")) != -1 ) {
    switch ( c ) 
      {
      case 'q':
	__volume = 0;
	break;
      case 'v':
	__volume = 2;
	break;
      default:
	usage(argv[0]);
      }
  }

  ulog_init(argv[0]);

  int rc = pass_http_messages(STDIN_FILENO, STDOUT_FILENO);

  free(__headers);
  free(__url);

  ulog_close();

  return rc;
}
