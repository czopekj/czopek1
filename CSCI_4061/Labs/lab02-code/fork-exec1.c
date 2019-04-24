#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void){

  char *child_argv[] = {"gcc","-ah", "..", NULL};
  char *child_cmd = "gcc";

  printf("Running command '%s'\n",child_cmd);
  printf("------------------\n");
  pid_t child_pid = fork();

  if(child_pid == 0) {
    execvp(child_cmd, child_argv);
  } else {
    int status;
    wait(&status);
  }
  printf("------------------\n");
  printf("Finished\n");
  return 0;
}
