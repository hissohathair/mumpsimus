/*
 * noop.c -- No operation performed.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "util.h"
#include "ulog.h"

int main(int argc, char *argv[])
{
  ssize_t br = 1;
  char    *buf = NULL;
  int     do_writes = 1;
  ssize_t rc = EX_OK;

  size_t total_read = 0;
  size_t total_wrote = 0;

  buf = (char*)malloc(sizeof(char) * BUFFER_MAX);
  if ( buf == NULL ) {
    perror("Error from malloc");
    return EX_OSERR;
  }

  ulog_init(argv[0]);

  if ( strcmp(argv[0] + strlen(argv[0]) - 4, "null") == 0 )
    do_writes = 0;
  ulog(LOG_INFO, "Starting %s (do_writes=%d; buffer size=%d)", argv[0], do_writes, BUFFER_MAX);

  do {
    memset(buf, 0, BUFFER_MAX);
    br = read(STDIN_FILENO, buf, BUFFER_MAX);
    ulog_debug("Read %zd bytes from fd=%d", br, STDIN_FILENO);

    if ( br > 0 )
      total_read += (size_t)br;
    else if ( br < 0 )
      rc = EX_IOERR;

    if ( do_writes && br >= 1 ) {
      ssize_t nw = write_all(STDOUT_FILENO, buf, br);
      ulog_debug("Wrote %zd bytes to fd=%d", nw, STDOUT_FILENO);
      if ( nw > 0 )
	total_wrote += (size_t)nw;
    }

  } while ( br > 0 );

  ulog(LOG_INFO, "EOF reached. Read %zd; Wrote %zd bytes. Exiting.", total_read, total_wrote);

  free(buf);
  ulog_close();

  return rc;
}
