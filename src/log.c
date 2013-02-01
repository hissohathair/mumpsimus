/* log.c -- print out features of a HTTP stream on stderr */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "http_parser.h"

#ifndef SSIZE_MAX
# define SSIZE_MAX 2048
#endif


int pass_through(const char *prog_name)
{
  int     rc = 0;
  char    *buf = NULL;
  ssize_t br = 1;

  buf = (char*)malloc(sizeof(char) * SSIZE_MAX);
  if ( buf == NULL ) {
    fprintf(stderr, "%s: Out of memory attempting to allocate %d bytes\n", prog_name, SSIZE_MAX);
    return -1;
  }


  br = 1;
  while ( br != 0 ) {
    memset(buf, 0, SSIZE_MAX);
    br = read(STDIN_FILENO, buf, SSIZE_MAX);
    if ( br > 1 ) {
      write(STDOUT_FILENO, buf, br);
    }
    else if ( br < 0 ) {
      fprintf(stderr, "%s: read error (%d -- %s)\n", prog_name, errno, strerror(errno));
      rc = 1;
    }
  }
  
  free(buf);
  return rc;
}


int main(int argc, char *argv[])
{
  int rc = 0;
  rc = pass_through(argv[0]);
  return rc;
}
