/*
 * util.h -- common useful functions for I/O etc.
 *
 * After including this file, the caller can
 *   - rely on a subset of sysexits.h defines existing (see below)
 *   - rely on memset (even if HAVE_MEMSET is still undefined)
 *   - use BUFFER_MAX for buffers sizing
 *   - config.h will have been included (if it exists)
 * As well as use the macro and function prototypes below.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <sys/types.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


/* 
 * Occassionally useful macros.
 */
#ifndef BUFFER_MAX
#  define BUFFER_MAX 32 * 1024
#endif
#ifndef URL_MAX
#  define URL_MAX 2048
#endif
#ifndef STRING_MAX
#  define STRING_MAX 1024
#endif
#ifndef LINE_MAX
#  define LINE_MAX 256
#endif


/*
 * Function prototypes.
 */
size_t upper_power_of_two(const size_t n);
ssize_t write_all(const int fd, const char *buf, const ssize_t bytes_to_write);
ssize_t pass_through(const int fd_in, const int fd_out);

#ifndef HAVE_STRLCAT
size_t strlcat(char *dst, const char *src, size_t siz);
#endif /* HAVE_STRLCAT */

#endif /* __UTIL_H__ */

