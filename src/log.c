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

int pass_through(int fd_in, int fd_out)
{
  int     rc = EX_OK;
  char    *buf = NULL;
  ssize_t br = 0;

  buf = (char*)malloc(sizeof(char) * SSIZE_MAX);
  if ( buf == NULL ) {
    perror("Error from malloc");
    return EX_OSERR;
  }


  do {
    memset(buf, 0, SSIZE_MAX);
    br = read(fd_in, buf, SSIZE_MAX);
    if ( br > 1 ) {
      write_all(fd_out, buf, br);
    }
    else if ( br < 0 ) {
      perror("Error reading data");
      rc = EX_IOERR;
    }
  } while ( br > 0 );
  
  free(buf);

  return rc;
}


int main(int argc, char *argv[])
{
  int rc = 0;
  rc = pass_through(STDIN_FILENO, STDOUT_FILENO);
  return rc;
}
