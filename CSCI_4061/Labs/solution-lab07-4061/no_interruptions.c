// no_interruptions.c: A C program that is not killed from interrupt
// or terminate signals by establishing handlers that ignore signals.
//
// To stop this program from running, open another terminal and try
//  > pkill -9 a.out
// assuming you named the output program a.out
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

// Function run when a SIGINT is sent to the program
void handle_SIGINT(int sig_num) {
  printf("\nNo SIGINT-erruptions allowed.\n");
  fflush(stdout);
}
 
// Function run when a SIGTERM is sent to the program
void handle_SIGTERM(int sig_num) {
  printf("\nTry to SIGTERM me? Piss off!\n");
  fflush(stdout);
}

int main () {
  // Set handling functions for programs
  struct sigaction my_sa = {};               // portable signal handling setup with sigaction()
  sigemptyset(&my_sa.sa_mask);               // don't block any other signals during handling
  my_sa.sa_flags = SA_RESTART;               // always restart system calls on signals possible 

  my_sa.sa_handler = handle_SIGTERM;         // run function handle_SIGTERM
  sigaction(SIGTERM, &my_sa, NULL);          // register SIGTERM with given action

  my_sa.sa_handler = handle_SIGINT;          // run function handle_SIGINT
  sigaction(SIGINT,  &my_sa, NULL);          // register SIGINT with given action
 
  // Infinite loop 
  while(1) {        
    sleep(1);
    printf("Ma-na na-na!\n");
    fflush(stdout);
  }
  return 0;
}
