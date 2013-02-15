/*
 * Common DEFINEs etc.
 */

#ifndef __MUMPSIMUS_H__
#define __MUMPSIMUS_H__

#include "config.h"

#ifdef HAVE_SYSEXITS_H
# include <sysexits.h>
#else
# define EX_OK 0
# define EX_USAGE 64
# define EX_UNAVAILABLE 69
# define EX_OSERR 71
#endif


/*
 * Used in pipe commns
 */
struct fd_pipe_set {
  int fd0, fd1, fd2;
} ;



/* 
 * Occassionally useful macros.
 */

#define LOG(x) fprintf(stderr, "%s(%d): %s\n", __FILE__, __LINE__, x)

#ifndef SSIZE_MAX
# define SSIZE_MAX 2048
#endif

#endif
