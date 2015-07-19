/*
 * body.c -- send the body of a HTTP message through a piped command.
 * 
 * This one has to work in a different way to headers.c That program
 * could get away with letting the piped command just send to stdout.
 * For bodies though we need to know the eventual content-length (or
 * maybe even strip out the Content-Length header and just close the
 * connection if the body is very long). The http_parser module is
 * converting chunked encoding.
 *
 * So this command will hold both read and write ends of the pipe
 * and read the piped commands output and then send to stdout once
 * the message has finished being read.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
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



/* Body_State
 *
 *    Holds state information between callbacks.
 *
 *    fd_stdin    File handle (int) to read, usually STDIN.
 *    fd_stdout   File handle to write to, usually STDOUT.
 *    url         Copy of last URL encountered.
 *    status_line Copy of last HTTP status line (res or req).
 *    headers     Stream buffer that stores headers.
 *    body        Stream buffer for storing piped (modified) body.
 *    ph          Pipe_Handle for communicating with pipe command.
 */
struct Body_State
{
  int fd_stdin;
  int fd_stdout;

  ssize_t content_length_at;
  bool last_field_was_content_type;
  bool do_pipe_this_message;

  char *url;
  char *status_line;
  char *content_type_pattern;

  struct Stream_Buffer *headers;
  struct Stream_Buffer *body;
  struct Pipe_Handle *ph;
};


/* body_state_init:
 *
 *    Initilises and allocates memory for Body_State fields. Aborts
 *    (terminates) program on malloc errors, otherwise returns void.
 */
struct Body_State *
body_state_new(const char *type_pattern)
{
  struct Body_State *bstate = malloc(sizeof(struct Body_State));

  bstate->fd_stdin = STDIN_FILENO;
  bstate->fd_stdout = STDOUT_FILENO;
  bstate->content_length_at = -1;
  bstate->last_field_was_content_type = false;
  bstate->do_pipe_this_message = true;

  if (NULL == type_pattern)
    bstate->content_type_pattern = NULL;
  else
    bstate->content_type_pattern = strdup(type_pattern);

  bstate->url = malloc(URL_MAX);
  bstate->status_line = malloc(LINE_MAX);
  if ((NULL == bstate->url) || (NULL == bstate->status_line))
    {
      perror("malloc() error initialising body state");
      abort();
    }
  bzero(bstate->url, URL_MAX);
  bzero(bstate->status_line, LINE_MAX);

  bstate->headers = stream_buffer_new();
  bstate->body = stream_buffer_new();
  if ((bstate->headers == NULL) || (bstate->body == NULL))
    {
      perror("stream_buffer_new() error initiaising body state");
      abort();
    }

  bstate->ph = pipe_handle_new();
  if (bstate->ph == NULL)
    {
      perror("pipe_handle_new() error initialising body state");
      abort();
    }

  return bstate;
}

/* body_state_delete:
 *
 *    Frees allocated memory. Returns void.
 */
void
body_state_delete(struct Body_State *bstate)
{
  free(bstate->url);
  bstate->url = NULL;
  free(bstate->status_line);
  bstate->status_line = NULL;

  if (NULL != bstate->content_type_pattern)
    {
      free(bstate->content_type_pattern);
      bstate->content_type_pattern = NULL;
    }

  stream_buffer_delete(bstate->headers);
  bstate->headers = NULL;
  stream_buffer_delete(bstate->body);
  bstate->body = NULL;
  pipe_handle_delete(bstate->ph);
  bstate->ph = NULL;

  free(bstate);
  return;
}



/* cb_url: 
 *
 *    Callback from http_parser, called when URL read. Preserves the
 *    URL for later.
 */
int
cb_url(http_parser * parser, const char *at, size_t length)
{
  struct Body_State *bstate = (struct Body_State *) parser->data;

  // Safely copy URL to buffer provided.
  if (length < URL_MAX)
    {
      strncpy(bstate->url, at, length);
      bstate->url[length] = '\0';
    }
  else
    {
      strncpy(bstate->url, at, URL_MAX - 1);
      bstate->url[URL_MAX - 1] = '\0';
      ulog(LOG_WARNING, "URL exceeded %zd bytes and was truncated", URL_MAX);
    }
  ulog_debug("Recorded URL of HTTP message: %s", bstate->url);
  return 0;
}


/* cb_status_complete:
 *
 *    Called when the HTTP status has been read. Take opportunity to
 *    record the URL (pointed to by "at").
 */
int
cb_status_complete(http_parser * parser, const char *at, size_t length)
{
  struct Body_State *bstate = (struct Body_State *) parser->data;

  // Reset headers
  bstate->content_length_at = -1;
  stream_buffer_clear(bstate->headers);

  // Safely copy URL to allocated space
  if (length < URL_MAX)
    {
      strncpy(bstate->url, at, length);
      bstate->url[length] = '\0';
    }
  else
    {
      strncpy(bstate->url, at, URL_MAX - 1);
      bstate->url[URL_MAX] = '\0';
    }
  ulog_debug("HTTP response status: %s", bstate->url);
  return 0;
}


/* cb_header_field:
 *
 *    Called when HTTP parser has read a header field. The parser will
 *    discard this but we need it to output it again. So we add it to
 *    our output (stream) buffer. Also need to add the colon and
 *    trailing space.
 */
#define CONTENT_LENGTH_STR "content-length"
#define CONTENT_LENGTH_LEN 14
#define CONTENT_TYPE_STR "content-type"
#define CONTENT_TYPE_LEN 12
int
cb_header_field(http_parser * parser, const char *at, size_t length)
{
  struct Body_State *bstate = (struct Body_State *) parser->data;

  ulog_debug("Header field: %.*s", (int)length, at);

  // If this header field is for Content-Length, remember where we are before appending.
  // If field is content-type, set flag for checking content type on header value.
  bstate->last_field_was_content_type = false;
  if (strncasecmp(CONTENT_LENGTH_STR, at, MIN(length, CONTENT_LENGTH_LEN)) == 0)
    {
      bstate->content_length_at = stream_buffer_size(bstate->headers);
    }
  else if (strncasecmp(CONTENT_TYPE_STR, at, MIN(length, CONTENT_TYPE_LEN)) == 0)
    {
      bstate->last_field_was_content_type = true;
    }

  // Now append headers to output buffer
  stream_buffer_add(bstate->headers, at, length);
  stream_buffer_add(bstate->headers, ": ", 2);
  return 0;
}


/* cb_header_value:
 *
 *    Companion to above function, called when a value for a header
 *    has been read. Append this to the same output stream
 *    buffer. Parser has stripped the trailing newline sequence so
 *    need to add that back.
 */
int
cb_header_value(http_parser *parser, const char *at, size_t length)
{
  struct Body_State *bstate = (struct Body_State *) parser->data;

  ulog_debug("Header value: %.*s", (int)length, at);

  stream_buffer_add(bstate->headers, at, length);
  stream_buffer_add(bstate->headers, "\r\n", 2);

  // Was last header field "content-type"? If so, match content type here
  if (bstate->last_field_was_content_type && (NULL != bstate->content_type_pattern))
    {
      char *temp_copy = strndup(at, length);
      if (fnmatch(bstate->content_type_pattern, temp_copy, FNM_CASEFOLD) == 0)
	{
	  bstate->do_pipe_this_message = true;
	}
      else
	{
	  bstate->do_pipe_this_message = false;
	}
      ulog_debug("Content type was [%s] matched against [%s] == %d", temp_copy, 
		 bstate->content_type_pattern, bstate->do_pipe_this_message);
      free(temp_copy);
    }

  return 0;
}


/* cb_headers_complete: 
 *     
 *    Called by http_parser_execute when the header has completed. But
 *    we can't send the headers to stdout yet, because the body may
 *    have changed size and we need to recalculate the Content-Length
 *    header.
 *
 *    Note: if we're not really going to pipe the body, because the
 *    user has specified that they want to match against a type, and
 *    the current message doesn't match that type, then we are going
 *    to dump headers as soon as the body starts getting written. But
 *    for now, we still accumulate headers into the stream buffer.
 *
 */
int
cb_headers_complete(http_parser * parser)
{
  struct Body_State *bstate = (struct Body_State *) parser->data;

  // Build the HTTP message start based on message type
  char *str = bstate->status_line;
  if (HTTP_REQUEST == parser->type)
    {
      snprintf(str, BUFFER_MAX, "%s %s HTTP/%d.%d\r\n",
	       http_method_str(parser->method),
	       bstate->url, parser->http_major, parser->http_minor);
    }
  else
    {
      snprintf(str, BUFFER_MAX, "HTTP/%d.%d %d %s\r\n",
	       parser->http_major,
	       parser->http_minor, parser->status_code, bstate->url);
    }
  ulog_debug("cb_headers_complete(parser=%X, parser->type=%d, do_pipe_this_message=%d): %s\n", parser,
	     parser->type, bstate->do_pipe_this_message, str);

  // Append blank line to end of HTTP headers.
  stream_buffer_add(bstate->headers, "\r\n", 2);

  return 0;
}


/* cb_body:
 *
 *    Called each time a chunk of the body has been read by the
 *    parser. Some conditional logic here:
 *      - If we are piping this output, We will write this to the 
 *        pipe write end. 
 *      - If we are not piping this output, just send to stdout.
 *
 *    If we're not piping, we'll need to write headers before writing
 *    this body chunk. 
 *
 *    TODO: The main parser loop will periodically poll the read end
 *    to make sure the piped command doesn't block on write because
 *    its output buffers are full.
 */
int
cb_body(http_parser * parser, const char *at, size_t length)
{
  struct Body_State *bstate = (struct Body_State *) parser->data;

  int fd = 0;
  if (bstate->do_pipe_this_message)
    {
      fd = pipe_write_fileno(bstate->ph);
    }
  else
    {
      fd = bstate->fd_stdout;
      // write headers if haven't already
      if (stream_buffer_size(bstate->headers) > 0) 
	{
	  write_all(fd, bstate->status_line, strlen(bstate->status_line));
	  stream_buffer_write(bstate->headers, fd);
	}
    }

  write_all(fd, at, length);

  ulog_debug
    ("cb_body(parser=%X, at=%X, length=%zd) -> fd=%d (do_pipe_this_message=%d)",
     parser, at, length, fd, bstate->do_pipe_this_message);

  return 0;
}



/* cb_message_complete:
 *
 *    Called by http_parser_execute when the HTTP message is all
 *    done. Need to:
 *        - Close our write-end of the pipe.
 *        - Read remaining data from read-end of the pipe.
 *        - Calculate the length of the body.
 *        - Output the headers, correcting the Content-Length field.
 *        - Output the body.
 *        - Reset the pipe.
 *        - Reset the parser.
 */
int
cb_message_complete(http_parser * parser)
{
  struct Body_State *bstate = (struct Body_State *) parser->data;


  // If we've been filtering then need to flush out the pipe
  if ( ! bstate->do_pipe_this_message)
    {
      ulog(LOG_INFO, "Message complete");
    }
  else
    {
      // Close the write end of the pipe so that it gets eof and flushes data
      pipe_send_eof(bstate->ph);

      // Read remaining data from read-end of pipe.
      char *buffer = malloc(BUFFER_MAX);
      if (buffer == NULL)
	{
	  perror("Error retured from malloc");
	  abort();
	}

      ssize_t bytes_read = 0;
      do
	{
	  bytes_read = read(pipe_read_fileno(bstate->ph), buffer, BUFFER_MAX);
	  if (bytes_read > 0)
	    stream_buffer_add(bstate->body, buffer, bytes_read);
	  else if (bytes_read < 0)
	    ulog(LOG_ERR, "read returned an error (%d): %s", errno,
		 strerror(errno));
	}
      while (bytes_read > 0);

      // Get the new body length
      size_t body_length = stream_buffer_size(bstate->body);

      // Output HTTP status message
      ulog_debug("Body length is %zd", body_length);
      write_all(bstate->fd_stdout, bstate->status_line,
	    strlen(bstate->status_line));
      
      if (bstate->content_length_at >= 0)
	{
	  // Output all headers up to (not including) Content-Length
	  stream_buffer_write_to(bstate->headers, bstate->fd_stdout,
				 bstate->content_length_at);
	  snprintf(buffer, BUFFER_MAX,
		   "Content-Length: %zd\r\nX-Mumpsimus-Original-",
		   body_length);
	  write_all(bstate->fd_stdout, buffer, strlen(buffer));

	  // Rest of headers buffer already includes blank line
	  stream_buffer_write(bstate->headers, bstate->fd_stdout);
	}
      else
	{
	  // No Content-Length header! TODO: Should we add a "Connection: close" here?
	  stream_buffer_write(bstate->headers, bstate->fd_stdout);
	}

      // Output the new body
      stream_buffer_write(bstate->body, bstate->fd_stdout);

      // Reset pipe
      pipe_reset(bstate->ph);

      // Free memory from this routine
      free(buffer);
    }

  // Reset parser
  http_parser_init(parser, HTTP_BOTH);
  bstate->content_length_at = -1;

  return 0;
}


/* pipe_http_messages:
 *
 *    Want to read from stdin ---> send it to pipe's stdin.  Read
 *    from pipe's stdout ---> send it to my stdout.
 *
 *    pipe_cmd      Shell command to open pipe to
 *    type_pattern  If not NULL, matched against Content-Type. Only
 *                  messages that match are sent to pipe_cmd.
 */
int
pipe_http_messages(int fd_in, int fd_out, const char *pipe_cmd,
		   const char *type_pattern)
{
  int errors = 0;
  int rc = EX_OK;


  // This struct will hold inforamtion we need on each callback
  struct Body_State *bstate = body_state_new(type_pattern);
  bstate->fd_stdin = fd_in;
  bstate->fd_stdout = fd_out;

  // This struct sets up callbacks for the HTTP parser
  http_parser_settings settings;
  http_parser_settings_init(&settings);
  settings.on_header_field = cb_header_field;
  settings.on_header_value = cb_header_value;
  settings.on_headers_complete = cb_headers_complete;
  settings.on_status = cb_status_complete;
  settings.on_message_complete = cb_message_complete;
  settings.on_url = cb_url;
  settings.on_body = cb_body;

  // Initialise a new HTTP parser
  http_parser parser;
  http_parser_init(&parser, HTTP_BOTH);
  parser.data = (void *) bstate;


  // Now need to open pipe command 
  if (pipe_open2(bstate->ph, pipe_cmd) != 0)
    {
      ulog(LOG_ERR, "Unable to open pipe to command: %s", pipe_cmd);
      return -1;
    }


  // Buffer for reading data from stdin
  char *buffer = malloc(BUFFER_MAX);
  if (NULL == buffer)
    {
      perror("Error from malloc");
      return -1;
    }


  ssize_t last_parsed = 0;
  ssize_t bytes_read = 0;
  char *buf_ptr = buffer;
  bool do_reads = true;
  do
    {
      // Read data from stdin
      memset(buffer, 0, BUFFER_MAX);
      buf_ptr = buffer;
      bytes_read = read(fd_in, buffer, BUFFER_MAX);
      ulog_debug("Read %zd bytes from fd=%d", bytes_read, fd_in);

      // Handle error or eof
      if (bytes_read < 0)
	{
	  errors++;
	  do_reads = false;
	  ulog(LOG_ERR, "Read returned error %d (%s)", errno,
	       strerror(errno));
	}
      else if (0 == bytes_read)
	{
	  // let http parser know we are done
	  last_parsed = http_parser_execute(&parser, &settings, buf_ptr, 0);
	  ulog(LOG_DEBUG, "Read returned EOF. Have told parser.");
	  do_reads = false;
	}
      else
	{
	  // Repeatedly call http_parser_execute while there is data left in the buffer
	  while ((bytes_read > 0) && (errors <= 0))
	    {

	      last_parsed =
		http_parser_execute(&parser, &settings, buf_ptr, bytes_read);
	      ulog_debug("Parsed %zd bytes out of %zd bytes remaining",
			 last_parsed, bytes_read);

	      if (parser.upgrade) // TODO: support this properly
		{
		  ulog(LOG_ERR, "HTTP Upgrade protocol invoked -- feature not supported");
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
		  ulog(LOG_ERR,
		       "Parser error reading HTTP stream type %d: %s (%s). Next char is %c (%d)",
		       parser.type,
		       http_errno_description(HTTP_PARSER_ERRNO(&parser)),
		       http_errno_name(HTTP_PARSER_ERRNO(&parser)), *buf_ptr,
		       *buf_ptr);
		}		// while (bytes_read > 0) && (errors <= 0)
	    }			// if...else
	}
      ulog_debug("Inner parser loop exited (bytes_read=%zd; errors=%d)",
		 bytes_read, errors);
    }
  while (do_reads && (errors <= 0));

  // We allocated these
  free(buffer);
  body_state_delete(bstate);

  if (errors > 0)
    rc = EX_IOERR;

  return rc;
}


/* usage: 
 *
 *    Command usage and exit. Does not return!
 */
void
usage(const char *ident, const char *message)
{
  if (message != NULL)
    fprintf(stderr, "error: %s\n", message);
  fprintf(stderr, "usage: %s -c command\n", ident);
  exit(EX_USAGE);
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
main(int argc, char *argv[])
{
  int rc = EX_OK;		// Exit code to return to environment
  char *pipe_cmd = NULL;	// Pointer to command line to pipe output through
  char *type_pattern = NULL;	// Pointer to MIME type glob pattern

  // TODO: Maybe add option to only do requests or responses?

  // Process command line arguments
  int c = 0;
  while ((c = getopt(argc, argv, "t:c:")) != -1)
    {
      switch (c)
	{
	case 'c':
	  if (NULL == optarg)
	    usage(argv[0], "Must pass a command to -c");
	  pipe_cmd = optarg;
	  break;
	case 't':
	  if (NULL == optarg)
	    usage(argv[0], "Must pass a type pattern to -t");
	  type_pattern = optarg;
	  break;
	case '?':
	  usage(argv[0], NULL);
	  break;
	}
    }

  // Sanity check environment
  if (NULL == pipe_cmd)
    {
      usage(argv[0],
	    "Must provide a command to pipe through with -c <command>");
    }

  if (system(NULL) == 0)
    {
      perror("System shell is not available");
      return EX_OSERR;
    }


  // Initialise logging tool and begin debug log message
  ulog_init(argv[0]);
  ulog(LOG_INFO, "Piping all HTTP message bodies through %s", pipe_cmd);
  if (NULL != type_pattern)
    ulog(LOG_INFO, "Matching Content-Type against %s", type_pattern);

  // Call main program loop
  rc =
    pipe_http_messages(STDIN_FILENO, STDOUT_FILENO, pipe_cmd, type_pattern);

  ulog_close();

  return rc;
}
