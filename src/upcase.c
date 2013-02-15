/* 
 * upcase.c -- convert all lower case chars to upper case (ASCII only)
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

int main(int argc, char *argv[])
{
  ssize_t br = 0;
  char    *buf = NULL;
  int     rc = EX_OK;

  buf = (char*)malloc(sizeof(char) * SSIZE_MAX);
  if ( buf == NULL ) {
    perror("Error from malloc");
    return EX_OSERR;
  }

  do {
    memset(buf, 0, SSIZE_MAX);
    br = read(STDIN_FILENO, buf, SSIZE_MAX);
    if ( br > 1 ) {
      for ( int i = 0; i <= br ; i++ ) 
	buf[i] = toupper(buf[i]);
      write_all(STDOUT_FILENO, buf, br);
    }
    else if ( br < 0 ) {
      rc = EX_IOERR;
    }
  } while ( br != 0 );

  return rc;
}
