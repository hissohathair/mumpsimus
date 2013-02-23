/* 
 * util.c -- utility functions used in common across commands.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


#ifndef HAVE_STRLCAT
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND TODD C. MILLER DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL TODD C. MILLER BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz)
{
  char *d = dst;
  const char *s = src;
  size_t n = siz;
  size_t dlen;
  
  /* Find the end of dst and adjust bytes left but don't go past end */
  while (n-- != 0 && *d != '\0')
    d++;
  dlen = d - dst;
  n = siz - dlen;
  
  if (n == 0)
    return(dlen + strlen(s));
  while (*s != '\0') {
    if (n != 1) {
      *d++ = *s;
      n--;
    }
    s++;
  }
  *d = '\0';
  
  return(dlen + (s - src));	/* count does not include NUL */
}
#endif /* HAVE_STRLCAT */
