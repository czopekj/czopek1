// Server code which contains a name/email pairs and will fulfill
// requests from a client through System V IPC message queues.
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>


// data the server has and clients want: pairings of name and email
char *data[][2] = {
  {"Chris Kauffman"       ,"kauffman@umn.edu"},
  {"Christopher Jonathan" ,"jonat003@umn.edu"},
  {"Amy Larson"           ,"larson@cs.umn.edu"},
  {"Chris Dovolis"        ,"dovolis@cs.umn.edu"},
  {"Dan Knights"          ,"knights@cs.umn.edu"},
  {"George Karypis"       ,"karypis@cs.umn.edu"},
  {"Steven Jensen"        ,"sjensen@cs.umn.edu"},
  {"Daniel Keefe"         ,"dfk@umn.edu"},
  {"Michael W. Whalen"    ,"whalen@cs.umn.edu"},
  {"Catherine Qi Zhao"    ,"qzhao@umn.edu"},
  {"Dan Challou"          ,"challou@cs.umn.edu"},
  {"Steven Wu"            ,"zsw@umn.edu"},
  {"Michael Steinbach"    ,"steinbac@cs.umn.edu"},
  {"Jon Weissman"         ,"jon@cs.umn.edu"},
  {"Victoria Interrante"  ,"interran@cs.umn.edu"},
  {"Shana Watters"        ,"watt0087@umn.edu"},
  {"James Parker"         ,"jparker@cs.umn.edu"},
  {"James Moen"           ,"moen0017@cs.umn.edu"},
  {"Daniel Giesel"        ,"giese138@umn.edu"},
  {"Jon Read"             ,"readx028@umn.edu"},
  {"Sara Stokowski"       ,"stoko004@umn.edu"},
  {NULL                   , NULL},
};

typedef struct {
  char client_queue_name[256];        // filename of queue on which to respond
  char query_name[256];               // look up this person's email
} request_t;

int signalled = 0;
void handle_signals(int sig_num){
  signalled = 1;
}

int main() {
  setvbuf(stdout, NULL, _IONBF, 0); 
  struct sigaction my_sa = {
    .sa_handler = handle_signals,                                 // run function handle_signals
  };
  sigaction(SIGTERM, &my_sa, NULL);                               
  sigaction(SIGINT,  &my_sa, NULL);                               

  printf("SERVER %5d: starting up\n", getpid());

  struct mq_attr attr = {
    .mq_maxmsg = 10,
    .mq_msgsize = sizeof(request_t),
  };
  mqd_t server_qd =
    mq_open("/em_server_q", O_CREAT|O_RDONLY|O_NONBLOCK, // non-blocking
            S_IRUSR|S_IWUSR, &attr);
  
  printf("SERVER %5d: created message queue, listening for requests\n", getpid());

  while(!signalled){
    request_t request;
    while( -1 == mq_receive(server_qd, (char *) &request, sizeof(request_t), NULL) ){
      // printf("SERVER %5d: no message available yet\n",getpid());
      if(signalled==1){
        break;
      }
    }
    if(signalled==1){
      break;
    }
    printf("SERVER %5d: received request {client_queue_name='%s' query_name='%s' }\n",
           getpid(), request.client_queue_name, request.query_name);

    char *email = "NOT FOUND";                                   // search for the name/email in the 'database' of records
    for(int i=0; data[i][0] != NULL; i++){
      if( strcmp(request.query_name, data[i][0])==0 ){
        email = data[i][1];                                      // found name, assigne email 
      }
    }

    printf("SERVER %5d: opening client queue '%s'\n", getpid(), request.client_queue_name);
    mqd_t client_qd = mq_open(request.client_queue_name, O_WRONLY);

    printf("SERVER %5d: writing email '%s' for query_name '%s'\n",
           getpid(), email, request.query_name);
    mq_send(client_qd, email, 256, 0);                            // reply with results to client

    printf("SERVER %5d: closing connection to queue '%s'\n", getpid(), request.client_queue_name);
    mq_close(client_qd);
  }

  printf("SERVER %5d: signalled, closing and unlinking queue\n", getpid());
  mq_close(server_qd);
  mq_unlink("/em_server_q");
  
  return 0;
}
