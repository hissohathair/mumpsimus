/*
 * noop.c -- No operation performed.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

int main(int argc, char *argv[])
{
  ssize_t br = 1;
  char    *buf = NULL;
  int     do_writes = 1;
  ssize_t rc = EX_OK;

  buf = (char*)malloc(sizeof(char) * SSIZE_MAX);
  if ( buf == NULL ) {
    perror("Error from malloc");
    return EX_OSERR;
  }

  if ( strcmp(argv[0] + strlen(argv[0]) - 4, "null") == 0 )
    do_writes = 0;

  do {
    memset(buf, 0, SSIZE_MAX);
    br = read(STDIN_FILENO, buf, SSIZE_MAX);
    if ( do_writes && br > 1 )
      write_all(STDOUT_FILENO, buf, br);
    else if ( br < 0 )
      rc = EX_IOERR;
  } while ( br > 0 );

  free(buf);

  return rc;
}
