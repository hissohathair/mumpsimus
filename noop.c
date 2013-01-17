/* noop.c -- no-operation performed. */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 1024

int main(int argc, char *argv[])
{
  ssize_t br = 1;
  char    buf[BUFSIZE];
  int     do_writes = 1;
  int     rc = 0;


  if ( strcmp(argv[0] + strlen(argv[0]) - 4, "null") == 0 )
    do_writes = 0;

  while ( br != 0 ) {
    memset(buf, 0, BUFSIZE);
    br = read(0, &buf, BUFSIZE);
    if ( do_writes && br > 1 )
      write(1, &buf, br);
    else if ( br < 0 )
      rc = 1;
  }

  exit(rc);
}
