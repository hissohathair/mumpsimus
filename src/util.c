/* util.c */

#include <stdio.h>
#include <unistd.h>

#include "util.h"


/* 
 * write_all: Will retry interrupted write(2) calls until either an error
 * occurs or all bytes successfully written.
 */
ssize_t write_all(const int fd, const char *buf, const ssize_t bytes_to_write)
{  
  ssize_t total_bytes = 0;
  ssize_t nw = 0;
  do {
    nw = write(fd, buf + total_bytes, bytes_to_write - total_bytes);
    if ( nw < 0 )
      perror("Could not write buffer");
    else 
      total_bytes += nw;
  } while ( (nw >= 0) && (total_bytes < bytes_to_write) );

  return total_bytes;
}


/* 
 * pass_through: Reading from fd_in, echo all bytes to fd_out until EOF.
 */
ssize_t pass_through(const int fd_in, const int fd_out)
{
  int     rc = EX_OK;
  char    *buf = NULL;
  ssize_t br = 0;
  ssize_t total_bytes = 0;

  buf = (char*)malloc(sizeof(char) * SSIZE_MAX);
  if ( buf == NULL ) {
    perror("Error from malloc");
    abort();
  }

  do {
    memset(buf, 0, SSIZE_MAX);
    br = read(fd_in, buf, SSIZE_MAX);
    if ( br > 0 ) {
      total_bytes += write_all(fd_out, buf, br);
    }
    else if ( br < 0 ) {
      perror("Error reading data");
      rc = EX_IOERR;
    }
  } while ( br > 0 );
  
  free(buf);

  return total_bytes;
}


void *memset_simple(void *b, int c, size_t len)
{
  int *buf = (int*)b;
  for ( int i = 0; i < len; i++ ) {
    buf[i] = c;
  }
  return b;
}
