/*
 * Forced re-education of UNIX pipes.
 *
 * Copyright Daniel Austin.
 *
 */

/*
 * Using style(9) guide. Synopsis for programmer:
 *     - include(s): kernel files (sys), user files (alpha), paths.h, local files
 *     - using sysexits.h for exit status codes
 *     - using paths.h for system files
 *     - gnu coding syntax style
 *
 * Revision history jokes hardly seem worth the effort.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event.h>

#include "mumpsimus.h"


struct event_base *init_event_base()
{
  struct event_base *base = NULL;
  const char **methods = NULL;

  base = event_base_new();
  event_base_priority_init(base, 1);

  methods = event_get_supported_methods();
  fprintf(stderr, "Running libevent version %s. Available methods: ", event_get_version());
  for (int i = 0; methods[i] != NULL; ++i) {
    fprintf(stderr, "%s ", methods[i]);
  }
  fprintf(stderr, " (using: %s)\n", event_base_get_method(base));

  return base;
}


void cb_stdin_ready(evutil_socket_t fd, short what, void *arg)
{
  struct fd_pipe_set *fds = arg;

  char buf[SSIZE_MAX];
  memset(buf, 0, SSIZE_MAX);

  printf("Got an event on socket %d:%s%s%s%s [flags = %d]\n",
	 (int) fd,
	 (what&EV_TIMEOUT) ? " timeout" : "",
	 (what&EV_READ)    ? " read" : "",
	 (what&EV_WRITE)   ? " write" : "",
	 (what&EV_SIGNAL)  ? " signal" : "",
	 fds->fd0);

  int nr = read(fd, &buf, SSIZE_MAX);
  if ( nr > 0 ) {
    int nw = 0;
    do {
      nw += write(fds->fd1, &buf, nr);
    } while ( nw < nr );

  }
}


int init_events(struct event_base *base)
{
  struct event       *ev1 = NULL, *ev2 = NULL;
  struct fd_pipe_set *fds = NULL;

  fds = (struct fd_pipe_set*)malloc(sizeof(struct fd_pipe_set));
  if ( NULL == fds ) {
    perror("Out of memory");
    abort();
  }
  fds->fd0 = 0;
  fds->fd1 = 1;
  fds->fd2 = 2;
  
  ev1 = event_new(base, fds->fd0, EV_READ, cb_stdin_ready, fds );
  event_add(ev1, NULL);

  return 0;
}


int main(int argc, char **argv)
{
  struct event_base *base =  NULL;

  base = init_event_base();
  init_events(base);
  event_base_dispatch(base);
  event_base_free(base);

  return EX_OK;
}
