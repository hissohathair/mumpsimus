/*
 * log.c -- print out features of a HTTP stream from stdin and report
 * on stderr.
 *
 * Not working yet. :)
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "ulog.h"
#include "util.h"
#include "http_parser.h"




/* usage: 
 *
 *    print usage message and abort. Does not return.
 */
void
usage (const char *ident)
{
  fprintf (stderr, "usage: %s [-qv]\n\t-q\tquiet\n\t-v\tverbose\n", ident);
  exit (EX_USAGE);
}




/* Use this struct with http_parser to persist some data
 * without resorting to globals.
 *
 *    url         String buffer to store last seen URL.
 *    headers     String buffer to store header fields and values.
 *    message     String buffer for last log message.
 *    volume      Remembers what level of logging caller wants.
 *
 * log_data_init allocates memory for the above string buffers.
 * log_data_free frees that same memory. Caller must malloc &
 * free the struct itself.
 */
struct Log_Data
{
  char *url;
  char *headers;
  char *message;
  int volume;			// 0=quiet; 1=normal; 2=verbose 
};

void
log_data_init (struct Log_Data *log_data)
{
  log_data->volume = 1;

  log_data->url = malloc (sizeof (char) * BUFFER_MAX);
  log_data->headers = malloc (sizeof (char) * BUFFER_MAX);
  log_data->message = malloc (sizeof (char) * BUFFER_MAX);
  if ((NULL == log_data->url) || (NULL == log_data->headers)
      || (NULL == log_data->message))
  {
    perror ("Error from malloc");
    abort ();
  }
  log_data->url[0] = '\0';
  log_data->headers[0] = '\0';
  return;
}

void
log_data_free (struct Log_Data *log_data)
{
  free (log_data->url);
  free (log_data->headers);
  free (log_data->message);
  log_data->url = NULL;
  log_data->headers = NULL;
  return;
}




/* log_highlight: 
 *
 *    Print terminal escape sequence to turn colour on and off. Checks
 *    the TERM environment for color compatible output device and only
 *    sends the ANSI escape sequence if it's pretty sure color is OK.
 *
 *    TODO: termcap does this wtf Dan how many kinds of wheel do you
 *    want...? 
 */
void
log_highlight (FILE * outf, const int turn_highlight_on)
{
  if (getenv ("TERM") == NULL)
    return;
  if (strstr (getenv ("TERM"), "color") == NULL)
    return;

  if (turn_highlight_on)
    fprintf (outf, "%c[1;32m", 27);	// green
  else
    fprintf (outf, "%c[0m", 27);	// white
}


/* cb_log_message_complete: 
 *
 *    Callback from http_parser, called when http message has been
 *    processed. A good time to extract some relevant log information.
 */
int
cb_log_message_complete (http_parser * parser)
{
  struct Log_Data *log_data = (struct Log_Data *) parser->data;
  char *str = log_data->message;

  // Build log message
  if (0 == parser->type)
  {
    snprintf (str, BUFFER_MAX, "[req] %s %s HTTP/%d.%d",
	      http_method_str (parser->method),
	      ((log_data->url == NULL
		|| !log_data->url[0]) ? "unknown" : log_data->url),
	      parser->http_major, parser->http_minor);
  }
  else
  {
    snprintf (str, BUFFER_MAX, "[res] HTTP/%d.%d %d %s",
	      parser->http_major,
	      parser->http_minor,
	      parser->status_code,
	      ((log_data->url == NULL
		|| !log_data->url[0]) ? "unknown" : log_data->url));
  }

  // Quiet (volume == 1) means log message type and basic status
  ulog (LOG_INFO, "cb_log_message_complete: %s (nread=%zd)", str,
	parser->nread);
  if (log_data->volume > 0)
  {
    log_highlight (stderr, 1);
    fprintf (stderr, "%s: %s\n", __FILE__, str);
    log_highlight (stderr, 0);
  }

  // Verbose (volume == 2) means log whole headers
  if (log_data->volume > 1)
    fprintf (stderr, "%s: [begin headers]\n%s%s: [end headers]\n", __FILE__,
	     log_data->headers, __FILE__);

  // Don't need these any more
  log_data->headers[0] = '\0';

  // Restart parser for next message type
  http_parser_init (parser, HTTP_BOTH);

  return 0;
}


/* cb_log_url: 
 *
 *    Callback from http_parser, called when URL read. Preserves the
 *    URL for later.
 */
int
cb_log_url (http_parser * parser, const char *at, size_t length)
{
  struct Log_Data *log_data = (struct Log_Data *) parser->data;
  size_t copylen = (length < BUFFER_MAX ? length : BUFFER_MAX);
  strncpy (log_data->url, at, copylen);
  log_data->url[copylen] = '\0';
  return 0;
}


/* cb_log_header_field: 
 *
 *    Called from http_parser, when a header field has been read. Adds
 *    this to our own buffer for later logging.
 */
int
cb_log_header_field (http_parser * parser, const char *at, size_t length)
{
  struct Log_Data *log_data = (struct Log_Data *) parser->data;

  char *new_field = strndup (at, length);
  strlcat (log_data->headers, "\t", BUFFER_MAX);
  strlcat (log_data->headers, new_field, BUFFER_MAX);
  strlcat (log_data->headers, ": ", BUFFER_MAX);
  free (new_field);
  return 0;
}


/* cb_log_header_value: 
 *
 *    Called from http_parser, when a header field's value has been
 *    read. Adds this to our own buffer for later logging.
 */
int
cb_log_header_value (http_parser * parser, const char *at, size_t length)
{
  struct Log_Data *log_data = (struct Log_Data *) parser->data;

  char *new_value = strndup (at, length);
  strlcat (log_data->headers, new_value, BUFFER_MAX);
  strlcat (log_data->headers, "\r\n", BUFFER_MAX);
  free (new_value);
  return 0;
}


/* cb_log_body
 *
 *    Called everytime the parser iterates over a HTTP message body. Nothing
 *    very interesting to do here except report the bytes.
 */
int
cb_log_body (http_parser * parser, const char *at, size_t length)
{
  //struct Log_Data *log_data = (struct Log_Data*)parser->data;
  ulog_debug ("cb_log_body called (len=%zd)", length);
  return 0;
}


/* pass_http_messages: 
 * 
 *    Reading from fd_in, echo bytes to fd_out and print "interesting"
 *    messages about the HTTP header being consumed.
 */
int
pass_http_messages (int fd_in, int fd_out, struct Log_Data *log_data)
{
  int rc = 0;
  int errors = 0;

  // Struct holds the callback settings for the parser
  http_parser_settings settings;
  http_parser_settings_init (&settings);
  settings.on_url = cb_log_url;
  settings.on_message_complete = cb_log_message_complete;
  settings.on_body = cb_log_body;
  if (log_data->volume > 1)
  {
    settings.on_header_field = cb_log_header_field;
    settings.on_header_value = cb_log_header_value;
  }

  // Struct holds parser instance, including our callback data
  http_parser parser;
  http_parser_init (&parser, HTTP_BOTH);
  parser.data = (void *) log_data;


  // Buffer for reading from stdin
  char *buffer = malloc (sizeof (char) * BUFFER_MAX);
  if (buffer == NULL)
  {
    perror ("Error from malloc");
    abort ();
  }

  ssize_t last_parsed = 0;
  ssize_t bytes_read = 0;
  char *buf_ptr = buffer;
  bool do_reads = true;
  do
  {
    // Read data from stdin
    memset (buffer, 0, BUFFER_MAX);
    buf_ptr = buffer;
    bytes_read = read (fd_in, buffer, BUFFER_MAX);
    ulog (LOG_DEBUG, "Read %zd bytes from fd=%d", bytes_read, fd_in);

    // Handle error or eof
    if (bytes_read < 0)
    {
      errors++;
      do_reads = false;
      ulog (LOG_ERR, "Read returned error %d (%s)", errno, strerror (errno));
    }
    else if (0 == bytes_read)
    {
      // let http parser know we are done
      last_parsed = http_parser_execute (&parser, &settings, buf_ptr, 0);
      ulog (LOG_DEBUG, "Read returned EOF. Have told parser.");
      do_reads = false;
    }
    else
    {
      // Repeatedly call http_parser_execute while there is data left in the buffer
      while ((bytes_read > 0) && (errors <= 0))
      {

	// We always echo to stdout directly what we've read
	write_all (fd_out, buffer, bytes_read);
	ulog (LOG_DEBUG, "Wrote %zd bytes to fd=%d", bytes_read, fd_out);

	// Call parser
	last_parsed =
	  http_parser_execute (&parser, &settings, buf_ptr, bytes_read);
	ulog (LOG_DEBUG, "Parsed %zd bytes from %zd\n", last_parsed,
	      bytes_read);

	// TODO: Have to handle connection upgrade
	if (parser.upgrade)
	{
	  ulog (LOG_ERR,
		"Parser requested HTTP connection upgrade but that's not implemented yet!");
	  errors++;
	}
	else if (last_parsed > 0)
	{
	  // Some data was parsed. Advance buffer.
	  bytes_read -= last_parsed;
	  buf_ptr += last_parsed;
	}
	else
	{
	  // Parser returned an error condition
	  errors++;
	  ulog (LOG_ERR,
		"Parser error reading HTTP stream type %d: %s (%s). Next char is %c (%d)",
		parser.type,
		http_errno_description (HTTP_PARSER_ERRNO (&parser)),
		http_errno_name (HTTP_PARSER_ERRNO (&parser)), *buf_ptr,
		*buf_ptr);
	  rc = EX_IOERR;
	}
      }				// while (bytes_read > 0) && (errors <= 0)
    }				// if...else
  }
  while (do_reads && (errors <= 0));

  // We allocated these
  free (buffer);

  return rc;
}


/* main: 
 *
 *    Check command line arguments, then beging processing. Returns 0
 *    if all OK, otherwise non-zero on parsing or other error.
 */
int
main (int argc, char *argv[])
{
  struct Log_Data log_data;
  log_data_init (&log_data);

  // Process command line arguments
  int c = 0;
  while ((c = getopt (argc, argv, "qv")) != -1)
  {
    switch (c)
    {
    case 'q':
      log_data.volume = 0;
      break;
    case 'v':
      log_data.volume = 2;
      break;
    default:
      usage (argv[0]);
    }
  }

  ulog_init (argv[0]);

  int rc = pass_http_messages (STDIN_FILENO, STDOUT_FILENO, &log_data);

  log_data_free (&log_data);
  ulog_close ();

  return rc;
}
