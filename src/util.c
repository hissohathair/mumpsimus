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


void *memset_simple(void *b, int c, size_t len)
{
  int *buf = (char*)b;
  for ( int i = 0; i < len; i++ ) {
    buf[i] = c;
  }
  return b;
}
