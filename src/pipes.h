#ifndef __PIPES_H__
#define __PIPES_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>

struct Pipe_Handle {
  int pipe_status;
  int pipe_fds[2];
  pid_t pipe_pid;
  const char *pipe_cmd;
};


void pipe_handle_init(struct Pipe_Handle *ph);
int pipe_open(struct Pipe_Handle *ph, const char *pipe_cmd);
int pipe_close(struct Pipe_Handle *ph);
int pipe_fileno(struct Pipe_Handle *ph);


#endif /* __ UTIL_H__*/
