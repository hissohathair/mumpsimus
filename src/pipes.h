#ifndef __PIPES_H__
#define __PIPES_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>


enum pipe_states { PIPES_CLOSED, PIPES_OPENING, PIPES_OPEN_UNI, PIPES_OPEN_BI, PIPES_WIDOWED };

struct Pipe_Handle {
  // Read-only
  unsigned int state;     // enum pipe_states
  pid_t child_pid;

  // Private
  int pipe_fds[2];
  int bipipe_fds[2];      // only used in bi-directional pipes
  char *command_line;
};


struct Pipe_Handle *pipe_handle_new(void);
void pipe_handle_delete(struct Pipe_Handle *ph);
int pipe_open(struct Pipe_Handle *ph, const char *pipe_cmd);
int pipe_open2(struct Pipe_Handle *ph, const char *pipe_cmd);
int pipe_close(struct Pipe_Handle *ph);
int pipe_reset(struct Pipe_Handle *ph);
void pipe_send_eof(struct Pipe_Handle *ph);
int pipe_write_fileno(struct Pipe_Handle *ph);
int pipe_read_fileno(struct Pipe_Handle *ph);


#endif /* __PIPES_H__*/
