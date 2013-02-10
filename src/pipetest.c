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
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <paths.h>

#define LOG(x) fprintf(stderr, "%s(%d): %s\n", __FILE__, __LINE__, x)
#ifndef SSIZE_MAX
# define SSIZE_MAX 2048
#endif

/* open_pipe: Will open a new pipe channel, fork, and then exec the
 *   command "cmd", using the standard shell to handle things like
 *   paths.
 * 
 */
int open_pipe(int p[2], char *cmd)
{
  int pid = 0;
  
  if( pipe(p) == -1 ) {
    perror("pipe() failed");
    return -1;
  }
  
  pid = fork();
  if ( -1 == pid ) {
    perror("fork() failed");
    return -1;
  }

  /* The child does the exec (pid==0). Parent returns */
  if ( 0 == pid ) {

    /* Close the child's read end of the pipe, and tie write end to stdout. */
    close(p[0]);
    dup2(p[1], 1);
    close(p[1]);

    /* Don't cache / buffer things at our end */
    fcntl(p[0], F_NOCACHE, 1);
    fcntl(p[1], F_NOCACHE, 1);

    /* Execute ps. */
    execl(_PATH_BSHELL, _PATH_BSHELL, "-c", cmd, NULL);
    perror("execl() failed");
    return -1;
  }

  return pid;
}


int main(int argc, char **argv)
{
  int pid_suck, p_suck[2];
  int pid_blow, p_blow[2];
  
  char buf[SSIZE_MAX];

  pid_blow = open_pipe(p_blow, "echo fred loves wilma loves barney loves betty loves fred");
  pid_suck = open_pipe(p_suck, "sed -El -e 's/ ([a-z]+) ([a-z]+) / \\2 \\1 /g'");
  LOG("open_pipe()s have returned");
  if ( pid_blow <= 0 || pid_suck <= 0 ) {
    fprintf(stderr, "%s: error -- unable to open pipes\n", argv[0]);
    exit(EX_UNAVAILABLE);
  }
    
  int child_status = 0;
  int num_childs = 2;

  while ( num_childs > 0 ) {
    int wait_status = waitpid(-1, &child_status, 0);
    num_childs--;

    if ( wait_status < 0 ) {
      perror("waitpid() failed");
    }
    else {
      fprintf(stderr, "%s: waitpid returned %d. ", argv[0], wait_status); 
      if ( WIFEXITED(child_status) )
 	fprintf(stderr, "Child terminated normally (status %d)\n", WEXITSTATUS(child_status));
      if ( WIFSIGNALED(child_status) ) 
	fprintf(stderr, "Child was signalled (signal %d)\n", WTERMSIG(child_status));
    }
  }

  int r = 0;
  fcntl(p_suck[0], F_SETFL, O_NONBLOCK); 
  while ( (r = read(p_suck[0], buf, SSIZE_MAX-1)) && (r > 0)) { 
    buf[r] = '\0'; 
    printf("SED [\n%s\n]\n", buf); 
  } 
  
  LOG("Aaaaaaaaaaand.... SCENE!"); 
  return(EX_OK); 
}
