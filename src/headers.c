/*
 * headers.c -- send the headers of a HTTP message through a piped command.
 *
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <paths.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "http_parser.h"
#include "util.h"
#include "ulog.h"
#include "stream_buffer.h"
#include "pipes.h"



struct headers_settings
{
  int fd_in;
  int fd_out;
  int fd_pipe;
  char *url;
  struct Stream_Buffer *sbuf;
  struct Pipe_Handle *ph;
};



/* cb_url: 
 *
 *    Callback from http_parser, called when URL read. Preserves the
 *    URL for later.
 */
int
cb_url (http_parser * parser, const char *at, size_t length)
{
  struct headers_settings *hset = (struct headers_settings *) parser->data;

  // Safely copy URL to buffer provided.
  if (length < URL_MAX)
  {
    strncpy (hset->url, at, length);
    hset->url[length] = '\0';
  }
  else
  {
    strncpy (hset->url, at, URL_MAX - 1);
    hset->url[URL_MAX - 1] = '\0';
    ulog (LOG_ERR, "URL exceeded %zd bytes and was truncated", URL_MAX);
  }
  ulog_debug ("Recorded URL of HTTP message: %s", hset->url);
  return 0;
}


/* cb_headers_complete: 
 *     
 *    Called by http_parser_execute when the header has completed. At
 *    this point we want to write the headers to the pipe command, and
 *    flush the output buffer by resetting the pipe.
 *
 */
int
cb_headers_complete (http_parser * parser)
{
  struct headers_settings *hset = (struct headers_settings *) parser->data;

  // Using a temporary string to build HTTP message start
  char *str = malloc (STRING_MAX);
  if (NULL == str)
  {
    perror ("Error in malloc");
    return -1;
  }

  // Build the HTTP message start based on message type
  if (HTTP_REQUEST == parser->type)
  {
    snprintf (str, STRING_MAX, "%s %s HTTP/%d.%d\r\n",
	      http_method_str (parser->method),
	      hset->url, parser->http_major, parser->http_minor);
  }
  else
  {
    snprintf (str, STRING_MAX, "HTTP/%d.%d %d %s\r\n",
	      parser->http_major,
	      parser->http_minor, parser->status_code, hset->url);
  }

  // Headers are written to the pipe
  int fd = hset->fd_pipe;
  ulog_debug ("cb_headers_complete(parser=%X, parser->type=%d) -> fd=%d",
	      parser, parser->type, fd);

  // Write the start of the HTTP message
  write_all (fd, str, strlen (str));

  // Append blank line to end of HTTP headers, then write the buffered HTTP
  // headers message.
  stream_buffer_add (hset->sbuf, "\r\n", 2);
  stream_buffer_write (hset->sbuf, fd);

  // Free mem & short pause... (TODO: Fix)
  free (str);
  usleep (7000);


  // Flush the headers by resetting the pipe
  if (pipe_reset (hset->ph) != 0)
  {
    perror ("Broken pipe");
    return -1;
  }

  return 0;
}

/* cb_message_complete:
 *
 *    Called by http_parser_execute when the HTTP message is all
 *    done. Need to reset the parser.
 */
int
cb_message_complete (http_parser * parser)
{
  ulog_debug ("HTTP message complete");
  http_parser_init (parser, HTTP_BOTH);
  return 0;
}


/* cb_status_complete:
 *
 *    Called when the HTTP status has been read. Take opportunity to
 *    record the URL (pointed to by "at").
 */
int
cb_status_complete (http_parser * parser, const char *at, size_t length)
{
  struct headers_settings *hset = (struct headers_settings *) parser->data;

  // Safely copy URL to allocated space
  if (length < URL_MAX)
  {
    strncpy (hset->url, at, length);
    hset->url[length] = '\0';
  }
  else
  {
    strncpy (hset->url, at, URL_MAX - 1);
    hset->url[URL_MAX] = '\0';
  }
  ulog_debug ("HTTP response status: %s", hset->url);
  return 0;
}


/* cb_header_field:
 *
 *    Called when HTTP parser has read a header field. The parser will
 *    discard this but we need it to output it again. So we add it to
 *    our output (stream) buffer. Also need to add the colon and
 *    trailing space.
 */
int
cb_header_field (http_parser * parser, const char *at, size_t length)
{
  struct headers_settings *hset = (struct headers_settings *) parser->data;
  stream_buffer_add (hset->sbuf, at, length);
  stream_buffer_add (hset->sbuf, ": ", 2);
  return 0;
}

/* cb_header_value:
 *
 *    Companion to above function, called when a value for a header
 *    has been read. Append this to the same output stream buffer. Parser
 *    has stripped the trailing newline sequence so need to add that.
 */
int
cb_header_value (http_parser * parser, const char *at, size_t length)
{
  struct headers_settings *hset = (struct headers_settings *) parser->data;
  stream_buffer_add (hset->sbuf, at, length);
  stream_buffer_add (hset->sbuf, "\r\n", 2);
  return 0;
}


/* cb_body:
 *
 *    Called each time a chunk of the body has been read by the
 *    parser.  We can just safely output this to stdout.
 */
int
cb_body (http_parser * parser, const char *at, size_t length)
{
  struct headers_settings *hset = (struct headers_settings *) parser->data;

  int fd = hset->fd_out;
  ulog_debug ("cb_body(parser=%X, at=%X, length=%zd) -> fd=%d", parser, at,
	      length, fd);

  write_all (fd, at, length);

  return 0;
}



/* pipe_http_messages:
 *
 *     Want to read from stdin ---> send it to pipe's stdin.  Read
 *     from pipe's stdout ---> send it to my stdout.
 */
int
pipe_http_messages (int fd_in, int fd_out, struct Pipe_Handle *ph)
{
  int errors = 0;
  int rc = EX_OK;


  // This struct will hold inforamtion we need on each callback
  struct headers_settings hset;
  hset.fd_in = fd_in;
  hset.fd_out = fd_out;
  hset.fd_pipe = pipe_write_fileno (ph);
  hset.url = malloc (URL_MAX);
  hset.sbuf = stream_buffer_new ();
  hset.ph = ph;
  if (NULL == hset.url || NULL == hset.sbuf)
  {
    perror ("Error in malloc or stream buffer");
    return -1;
  }
  hset.url[0] = '\0';

  // This struct sets up callbacks for the HTTP parser
  http_parser_settings settings;
  http_parser_settings_init (&settings);
  settings.on_header_field = cb_header_field;
  settings.on_header_value = cb_header_value;
  settings.on_headers_complete = cb_headers_complete;
  settings.on_status = cb_status_complete;
  settings.on_message_complete = cb_message_complete;
  settings.on_url = cb_url;
  settings.on_body = cb_body;

  // Initialise a new HTTP parser
  http_parser parser;
  http_parser_init (&parser, HTTP_BOTH);
  parser.data = (void *) &hset;


  // Buffer for reading data from stdin
  char *buffer = malloc (BUFFER_MAX);
  if (NULL == buffer)
  {
    perror ("Error from malloc");
    return -1;
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
    ulog_debug ("Read %zd bytes from fd=%d", bytes_read, fd_in);

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

	last_parsed =
	  http_parser_execute (&parser, &settings, buf_ptr, bytes_read);
	ulog_debug ("Parsed %zd bytes out of %zd bytes remaining",
		    last_parsed, bytes_read);

	// TODO: Have to handle connection upgrade
	if (parser.upgrade)
	{
	  ulog (LOG_ERR,
		"Parser requested HTTP connection upgrade but that's not implemented yet!");
	  errors++;
	}
	else if (last_parsed > 0)
	{
	  // Some data was parsed. Advance to next part of buffer
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
	}			// while (bytes_read > 0) && (errors <= 0)
      }				// if...else
    }
    ulog_debug ("Inner parser loop exited (bytes_read=%zd; errors=%d)",
		bytes_read, errors);
  }
  while (do_reads && (errors <= 0));

  // We allocated these
  free (buffer);
  free (hset.url);
  hset.url = NULL;
  stream_buffer_delete (hset.sbuf);

  if (errors > 0)
    rc = EX_IOERR;

  return rc;
}


/* usage: 
 *
 *    Command usage and abort. Does not return!
 */
void
usage (const char *ident, const char *message)
{
  if (message != NULL)
    fprintf (stderr, "error: %s\n", message);
  fprintf (stderr, "usage: %s -c command\n", ident);
  exit (EX_USAGE);
}


/* main: 
 *
 *    Parse the command line arguments and set up for main loop. Also
 *    sanity checks the environment.
 *
 * Returns
 *    0       All OK
 *    >=1     Error code returned to environment.
 */
int
main (int argc, char *argv[])
{
  int rc = EX_OK;		// Exit code to return to environment
  char *pipe_cmd = NULL;	// Pointer to command line to pipe output through


  // Process command line arguments
  int c = 0;
  while ((c = getopt (argc, argv, "c:")) != -1)
  {
    switch (c)
    {
    case 'c':
      if (NULL == optarg)
	usage (argv[0], "Must pass a command to -c");
      pipe_cmd = optarg;
      break;
    case '?':
      usage (argv[0], NULL);
      break;
    }
  }

  // Sanity check environment
  if (NULL == pipe_cmd)
  {
    usage (argv[0],
	   "Must provide a command to pipe through with -c <command>");
  }

  if (system (NULL) == 0)
  {
    perror ("System shell is not available");
    return EX_OSERR;
  }


  // Initialise logging tool and begin debug log message
  ulog_init (argv[0]);
  ulog (LOG_INFO, "Piping all HTTP headers through %s", pipe_cmd);


  struct Pipe_Handle *ph = pipe_handle_new ();
  if (ph == NULL)
  {
    perror ("Error returned from pipe_handle_new");
    return EX_OSERR;
  }

  if (pipe_open (ph, pipe_cmd) != 0)
  {
    ulog (LOG_ERR, "%s: Unable to open pipe to: %s", argv[0], pipe_cmd);
    rc = EX_UNAVAILABLE;
  }
  else
  {
    ulog (LOG_INFO, "%s: Opened pipe to: %s", argv[0], pipe_cmd);
    rc = pipe_http_messages (STDIN_FILENO, STDOUT_FILENO, ph);
    pipe_close (ph);
    pipe_handle_delete (ph);
  }

  ulog_close ();

  return rc;
}
