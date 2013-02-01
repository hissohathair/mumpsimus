#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BUFSIZE 20

int main(int argc, char **argv)
{
  int pid, r;
  int p[2];

  char buf[BUFSIZE];

  /* Pipe. */
  if(pipe(p) == -1)
    {
      fprintf(stderr, "pipe() failed.\n");
      exit(1);
    }
  
  /* Make a child (but never on the first date). */
  if((pid = fork()) == -1)
    {
      fprintf(stderr, "fork() failed.\n");
      exit(1);
    }

  /* If we're the child. */
  if(pid == 0) {

    /* Close the read end of the pipe. */
    close(p[0]);

    /* Tie the write end of the pipe to stdout. */
    dup2(p[1], 1);
    close(p[1]);

    /* Execute ps. */
    fprintf(stderr, "%s: child here, gonna get me some sed\n", argv[0]);
    execlp("sed", "sed", "-El", "-e", "s/ ([a-z]+) ([a-z]+) / \\2 \\1 /g", NULL);
    fprintf(stderr, "%s: we ran right past exec (%d=%s)!\n", argv[0], errno, strerror(errno));
  }

  /* If we're the father. */
  if(pid > 0) {

    fprintf(stderr, "%s: big daddy here. gonna process me some sed\n", argv[0]);
    
    /* Make p[0] nonblocking. */
    fcntl(p[0], F_SETFL, O_NONBLOCK);
    
    while((r = read(p[0], buf, BUFSIZE-1)) && (r > 0))
      {
	buf[r] = '\0';
	printf("SED [%s]", buf);
      }
    

    fprintf(stderr, "%s: big daddy to daddy mac, we're all done here. gonna reap some child process\n", argv[0]);

    int child_status = 0;
    int wait_status =0;
    wait_status = waitpid(pid, &child_status, 0);
    if ( wait_status < 0 ) {
      fprintf(stderr, "%s: waitpid was not happy, Jan (%s)\n", argv[0], strerror(errno));
    }
    else {
      fprintf(stderr, "%s: waitpid returned %d (pid=%d). child status was %d\n", argv[0], wait_status, pid,
	      child_status);
      if ( WIFEXITED(child_status) )
	fprintf(stderr, "%s:\tchild terminated normally (status %d)\n", argv[0], WEXITSTATUS(child_status));
      if ( WIFSIGNALED(child_status) )
	fprintf(stderr, "%s:\tchild was signalled (signal %d)\n", argv[0], WTERMSIG(child_status));
    }
  }


  fprintf(stderr, "%s: Aaaaaaaaaaand.... SCENE!\n", argv[0]);
  return(0);
}
