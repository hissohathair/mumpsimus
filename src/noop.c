/* noop.c -- no-operation performed. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifndef SSIZE_MAX
#define SSIZE_MAX 2048
#endif


int main(int argc, char *argv[])
{
  ssize_t br = 1;
  char    *buf = NULL;
  int     do_writes = 1;
  int     rc = 0;

  buf = (char*)malloc(sizeof(char) * SSIZE_MAX);
  if ( buf == NULL ) {
    fprintf(stderr, "%s: Out of memory\n", argv[0]);
    return 255;
  }

  if ( strcmp(argv[0] + strlen(argv[0]) - 4, "null") == 0 )
    do_writes = 0;

  while ( br != 0 ) {
    memset(buf, 0, SSIZE_MAX);
    br = read(0, buf, SSIZE_MAX);
    if ( do_writes && br > 1 )
      write(1, buf, br);
    else if ( br < 0 )
      rc = 1;
  }

  free(buf);

  return rc;
}
