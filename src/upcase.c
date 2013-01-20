/* upcase.c -- convert all lower case chars to upper case (ASCII only) */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFSIZE 1024

int main(int argc, char *argv[])
{
  ssize_t br = 1;
  char    buf[BUFSIZE];
  int     rc = 0;


  while ( br != 0 ) {
    memset(buf, 0, BUFSIZE);
    br = read(0, &buf, BUFSIZE);
    if ( br > 1 ) {
      for ( int i = 0; i <= br ; i++ ) 
	buf[i] = toupper(buf[i]);
      write(1, &buf, br);
    }
    else if ( br < 0 ) {
      rc = 1;
    }
  }

  exit(rc);
}
