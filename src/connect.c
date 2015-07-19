/*
 * connect -- fetch a resource
 */


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include "http_parser.h"
#include "ulog.h"
#include "util.h"


struct Http_Request 
{
  int fileno;
  char *url;
  char *current_header;
  struct curl_slist *headers_slist;
};



/* cb_header_field:
 *
 *    Called when HTTP parser has read a header field. It gets discarded
 *    so we need to add it to the header list for CURL.
 */
int
cb_header_field(http_parser * parser, const char *at, size_t length)
{
  struct Http_Request *req = (struct Http_Request*) parser->data;

  // Add to buffer
  if (length < STRING_MAX)
    {
      strncpy(req->current_header, at, length);
      req->current_header[length] = '\0';
    }
  else
    {
      strncpy(req->current_header, at, STRING_MAX-1);
      req->current_header[STRING_MAX-1] = '\0';
      ulog(LOG_ERR, "Header field too big (%zd > %d)-- truncated", length, STRING_MAX);
    }
  ulog_debug("New header: %s", req->current_header);
  return 0;
}

int
cb_header_value(http_parser * parser, const char *at, size_t length)
{
  struct Http_Request *req = (struct Http_Request*) parser->data;

  char *tmpstr = malloc(STRING_MAX);
  snprintf(tmpstr, STRING_MAX, "%s: %.*s", req->current_header, (int)length, at);
  req->headers_slist = curl_slist_append(req->headers_slist, tmpstr);
  free(tmpstr);

  return 0;
}

int
cb_url(http_parser * parser, const char *at, size_t length)
{
  struct Http_Request *req = (struct Http_Request*) parser->data;

  if ( length < URL_MAX )
    {
      strncpy(req->url, at, length);
      req->url[length] = '\0';
    }
  else
    {
      // TODO: This may be fatal to our request. Bomb out?
      strncpy(req->url, at, URL_MAX-1);
      req->url[URL_MAX-1] = '\0';
      ulog(LOG_ERR, "URL exceeded %zd bytes and was truncated", URL_MAX);
    }
  return 0;
}


/* read_request:
 *  
 *    Read a HTTP request message from file handle *fileno* and return a
 *    struct representing that object. Will return NULL on error.
 */
struct Http_Request *
read_request_new(const int fileno)
{
  // Allocate memory for struct
  struct Http_Request *req = malloc(sizeof(struct Http_Request));
  if ( NULL == req ) 
    {
      perror("malloc");
      abort();
    }

  req->url = malloc(URL_MAX);
  req->current_header = malloc(STRING_MAX);
  if ((NULL == req->url) || (NULL == req->current_header))
    {
      perror("malloc");
      abort();
    }

  req->fileno = fileno;
  req->headers_slist = NULL;


  // Parser object to parse incoming HTTP message
  http_parser_settings settings;
  http_parser_settings_init(&settings);
  settings.on_header_field = cb_header_field;
  settings.on_header_value = cb_header_value;
  settings.on_url = cb_url;

  http_parser parser;
  http_parser_init(&parser, HTTP_REQUEST);
  parser.data = (void *) req;


  // Allocate buffers and begin main loop
  char *buffer = malloc(BUFFER_MAX);
  if (NULL == buffer)
    {
      perror("malloc() error allocating read buffer");
      abort();
    }
  
  int errors = 0;
  ssize_t last_parsed = 0;
  ssize_t bytes_read = 0;
  char *buf_ptr = buffer;
  bool do_reads = true;
  do
    {
      // Read data from file handle (prob stdin)
      memset(buffer, 0, BUFFER_MAX);
      buf_ptr = buffer;
      bytes_read = read(fileno, buffer, BUFFER_MAX);
      ulog_debug("Read %zd bytes from fd=%d", bytes_read, fileno);

      // Handle error or eof
      if (bytes_read < 0)
	{
	  errors++;
	  do_reads = false;
	  ulog(LOG_ERR, "Read returned error %d (%s)", errno, strerror(errno));
	}
      else if (0 == bytes_read)
	{
	  // Let parser know we're done
	  last_parsed = http_parser_execute(&parser, &settings, buf_ptr, 0);
	  do_reads = false;
	  ulog(LOG_DEBUG, "Read returned EOF. Have told parser.");
	}
      else 
	{
	  // Repeatedly call http_parser_execute while there is data left in the buffer
	  while ((bytes_read > 0) && (errors <= 0))
	    {
	      last_parsed = http_parser_execute(&parser, &settings, buf_ptr, bytes_read);
	      ulog_debug("Parsed %zd bytes out of %zd bytes remaining", last_parsed, bytes_read);

	      if (parser.upgrade) // TODO: support this properly
		{
		  ulog(LOG_ERR, "HTTP Upgrade protocol invoked -- feature not supported");
		  errors++;
		}
	      else if (last_parsed >0)
		{
		  // At least some data was parsed. Advanced to next part of buffer
		  bytes_read -= last_parsed;
		  buf_ptr += last_parsed;
		}
	      else
		{
		  errors++;
		  ulog(LOG_ERR,
		       "Parser error reading HTTP stream type %d: %s (%s). Next char is %c (%d)",
		       parser.type,
		       http_errno_description(HTTP_PARSER_ERRNO(&parser)),
		       http_errno_name(HTTP_PARSER_ERRNO(&parser)), *buf_ptr,
		       *buf_ptr);
		}
	    } // while bytes_read >= 0
	}
    }
  while (do_reads && (errors <= 0));

  return req;
}


void
read_request_done(struct Http_Request *req)
{
  curl_slist_free_all(req->headers_slist);
  free(req->url);
  free(req->current_header);
  free(req);
}


CURLcode
fetch_request(struct Http_Request *req)
{
  CURLcode ret;
  CURL *hnd = NULL;

  ulog_debug("Requesting: %s ", req->url);

  hnd = curl_easy_init();
  curl_easy_setopt(hnd, CURLOPT_URL, req->url);
  curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
#ifdef CURLOPT_PATH_AS_IS
  curl_easy_setopt(hnd, CURLOPT_PATH_AS_IS, 1L);
#endif
  curl_easy_setopt(hnd, CURLOPT_HEADER, 1L);
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, req->headers_slist);
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 0L);
  curl_easy_setopt(hnd, CURLOPT_COOKIEFILE, "cookies.txt");
  curl_easy_setopt(hnd, CURLOPT_COOKIEJAR, "cookies.txt");
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);

  ret = curl_easy_perform(hnd);

  curl_easy_cleanup(hnd);
  hnd = NULL;
  ulog_debug("Done: %d", ret);

  return ret;
}


int
main(int argc, char *argv[])
{
  CURLcode ret;
  struct Http_Request *req = NULL;


  // Read a request
  req = read_request_new(STDIN_FILENO);

  // Take care of any spoofing, etc

  // Preserve the "special" headers

  // Perform the request
  ret = fetch_request(req);

  // Re-insert the special headers

  // Send result to stdout


  read_request_done(req);

  return (int) ret;
}


