/*
 * util.h -- common useful functions for I/O etc.
 *
 * After including this file, the caller can
 *   - rely on a subset of sysexits.h defines existing (see below)
 *   - rely on memset (even if HAVE_MEMSET is still undefined)
 *   - use SSIZE_MAX for buffers sizing
 *   - config.h will have been included (if it exists)
 * As well as use the macro and function prototypes below.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYSEXITS_H
# include <sysexits.h>
#else
# define EX_OK 0
# define EX_USAGE 64
# define EX_UNAVAILABLE 69
# define EX_OSERR 71
# define EX_IOERR 74
#endif


/* 
 * Occassionally useful macros.
 */
#define LOG(x) fprintf(stderr, "%s(%d): %s\n", __FILE__, __LINE__, x)
#ifndef SSIZE_MAX
# define SSIZE_MAX 2048
#endif


/*
 * Function prototypes.
 */
ssize_t write_all(const int fd, const char *buf, const ssize_t bytes_to_write);
ssize_t pass_through(const int fd_in, const int fd_out);
void *memset_simple(void *b, int c, size_t len);
#ifndef HAVE_MEMSET
# define memset(a,b,c) memset_simple(a,b,c)
#endif

#endif
