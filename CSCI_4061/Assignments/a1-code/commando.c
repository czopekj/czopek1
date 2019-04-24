#include "commando.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    cmdcol_t* global_cmdcol = malloc(sizeof(cmdcol_t));
    global_cmdcol->size = 0;

    // COMMANDO_ECHO environment variable/flag
    int COMMANDO_ECHO = 0;
    if (argv[1] != NULL) {
      if (strncmp(argv[1], "--echo", MAX_LINE) == 0) {
          COMMANDO_ECHO = 1;
      }
    }

    while (1) {
        printf("@> ");
        char terminal_input[MAX_LINE];
        // get line input from terminal and stores into terminal_input
        if (fgets(terminal_input, MAX_LINE, stdin) == NULL) {
          printf("\nEnd of input");
          break;
        } else {
          // if COMMANDO_ECHO is true, then first print terminal input
          if (COMMANDO_ECHO == 1) {
              printf("%s", terminal_input);
          }

          // parse terminal_input into tokens char array
          char** tokens = malloc(sizeof(char) * MAX_CMDS);
          int* num_tokens = 0;
          parse_into_tokens(terminal_input, tokens, num_tokens);

          char* first_token = tokens[0];                                              // declare first token initially to improve locality
          if (first_token == NULL) {                                                  // if user presses enter to exit, break while loop
              free(tokens);
              free(num_tokens);
              continue;
          } else if (strncmp(first_token, "help", MAX_LINE) == 0) {                   // if user enters "help", print given text
              printf("COMMANDO COMMANDS\n");
              printf("help               : show this message\n");
              printf("exit               : exit the program\n");
              printf("list               : list all jobs that have been started giving information on each\n");
              printf("pause nanos secs   : pause for the given number of nanseconds and seconds\n");
              printf("output-for int     : print the output for given job number\n");
              printf("output-all         : print output for all jobs\n");
              printf("wait-for int       : wait until the given job number finishes\n");
              printf("wait-all           : wait for all jobs to finish\n");
              printf("command arg1 ...   : non-built-in is run as a job\n");
          } else if (strncmp(first_token, "list", MAX_LINE) == 0) {
              // list all in cmdcol...
              cmdcol_print(global_cmdcol);
          } else if (strncmp(first_token, "exit", MAX_LINE) == 0) {                   // if user enters "exit", break while loop
              free(tokens);
              free(num_tokens);
              break;
          } else if (strncmp(first_token, "pause", MAX_LINE) == 0) {
              char* str_remainder;
              long tok1_toLong = strtol(tokens[1], &str_remainder, 10);                // use strtol() to convert string to long value
              int tok2_toInt = atoi(tokens[2]);                                       // convert tokens[2] to int
              pause_for(tok1_toLong, tok2_toInt);
          } else if (strncmp(first_token, "output-for", MAX_LINE) == 0) {             // if calls output-for
              int job_num = atoi(tokens[1]);
              // check if job number exists to prevent segmentation fault...
              if (job_num >= global_cmdcol->size) {
                perror("cannot use output-for due to invalid job number\n");
              } else {
                cmd_t* output_cmd = global_cmdcol->cmd[job_num];                        // find correct cmd given int index
                printf("@<<< Output for %s[#%d] (%d bytes):\n", output_cmd->name, output_cmd->pid, output_cmd->output_size);
                printf("----------------------------------------\n");
                cmd_print_output(output_cmd);                                           // call cmd_print_output to print output
                printf("----------------------------------------\n");
              }
          } else if (strncmp(first_token, "output-all", MAX_LINE) == 0) {
              cmd_t* current_cmd;
              for (int i = 0; i < global_cmdcol->size; i++) {
                  current_cmd = global_cmdcol->cmd[i];                                // find correct cmd given int index
                  printf("@<<< Output for %s[#%d] (%d bytes):\n", current_cmd->name, current_cmd->pid, current_cmd->output_size);
                  printf("----------------------------------------\n");
                  cmd_print_output(current_cmd);                                      // call cmd_print_output to print output
                  printf("----------------------------------------\n");
              }
          } else if (strncmp(first_token, "wait-for", MAX_LINE) == 0) {
              int job_num = atoi(tokens[1]);
              // check if job number exists to prevent any segmentation fault...
              if (job_num >= global_cmdcol->size) {
                perror("cannot use output-for due to invalid job number\n");
              } else {
                cmd_update_state(global_cmdcol->cmd[job_num], DOBLOCK);               // this calls to waitpid() for given child process and prints output message
              }
          } else if (strncmp(first_token, "wait-all", MAX_LINE) == 0) {
              for (int i = 0; i < global_cmdcol->size; i++) {
                  cmd_update_state(global_cmdcol->cmd[i], DOBLOCK);                   // iterate through all cmds in cmdcol. call cmd_update_state on all.
              }
          } else {                                                                    // no built-ins match, create new cmd_t and start it
              cmd_t* new_cmd = cmd_new(tokens);                                       // initialize new cmd_t
              cmdcol_add(global_cmdcol, new_cmd);                                     // add new cmd_t to global cmdcol
              cmd_start(new_cmd);                                                     // start running cmd_t
          }

          cmdcol_update_state(global_cmdcol, NOBLOCK);                                // update state of all child processes at end of each input loop
          free(tokens);                                                               // free tokens
          free(num_tokens);
      }
    }

    cmdcol_freeall(global_cmdcol);                                                  // free all dynamically allocated memory
    free(global_cmdcol);
}
