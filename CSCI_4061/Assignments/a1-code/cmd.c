// cmd.c: functions related the cmd_t struct abstracting a
// command. Most functions maninpulate cmd_t structs.

#include <stdio.h>
#include <string.h>
#include "commando.h"

#define NAME_MAX 255            // max len of commands and args
#define ARG_MAX 255             // max number of arguments
#define STATUS_LEN 10           // length of the str_status field in childcmd

// Allocates a new cmd_t with the given argv[] array. Makes string
// copies of each of the strings contained within argv[] using
// strdup() as they likely come from a source that will be
// altered. Ensures that cmd->argv[] is ended with NULL. Sets the name
// field to be the argv[0]. Sets finished to 0 (not finished yet). Set
// str_status to be "INIT" using snprintf(). Initializes the remaining
// fields to obvious default values such as -1s, and NULLs.
cmd_t *cmd_new(char *argv[]) {
    // allocate space for the new cmd
    cmd_t *new_cmd_t = malloc(sizeof(cmd_t));

    // copy argv into the new cmd's argv member
    int index = 0;
    while (argv[index] != NULL) {
        new_cmd_t->argv[index] = strdup(argv[index]);
        index++;
    }

  //  new_cmd_t->argv = *new_argv;
    new_cmd_t->argv[index] = NULL;                  // add null terminator at end of argv

    new_cmd_t->out_pipe[PWRITE] = -1;                   // set out_pipe write side to -1
    new_cmd_t->out_pipe[PREAD] = -1;                    // set out_pipe read side to -1

    strcpy(new_cmd_t->name, new_cmd_t->argv[0]);        // set name to argv[0]
    new_cmd_t->finished = 0;                            // set finished to 0
    new_cmd_t->status = -1;                             // set status to -1
    new_cmd_t->pid = -1;

    snprintf(new_cmd_t->str_status, STATUS_LEN, "%s", "INIT");

    new_cmd_t->output = NULL;                           // set output buffer to NULL
    new_cmd_t->output_size = -1;
}

// Deallocates a cmd structure. Deallocates the strings in the argv[]
// array. Also deallocats the output buffer if it is not
// NULL. Finally, deallocates cmd itself.
void cmd_free(cmd_t *cmd) {
    // free strings in the array
    int i;
    for (i = 0; cmd->argv[i] != NULL; i++) {
        free(cmd->argv[i]);
    }

    // if output buffer is not NULL, free it
    if (cmd->output != NULL) {
        free(cmd->output);
    }

    // lastly, free cmd_t
    free(cmd);
}

// Forks a process and starts executes command in cmd in the process.
// Changes the str_status field to "RUN" using snprintf().  Creates a
// pipe for out_pipe to capture standard output.  In the parent
// process, ensures that the pid field is set to the child PID. In the
// child process, directs standard output to the pipe using the dup2()
// command. For both parent and child, ensures that unused file
// descriptors for the pipe are closed (write in the parent, read in
// the child).
void cmd_start(cmd_t *cmd) {
    // create a pipe
    int result = pipe(cmd->out_pipe);

    // change str_status to RUN using snprintf
    snprintf(cmd->str_status, STATUS_LEN, "%s", "RUN");

    // For a new process and capture PID
    pid_t child_pid = fork();
    cmd->pid = child_pid;

    if (child_pid == 0) {
        // this is child
        dup2(cmd->out_pipe[PWRITE], STDOUT_FILENO);
        execvp(cmd->name, cmd->argv);
        close(cmd->out_pipe[PREAD]);
    } else {
        // this is parent
        close(cmd->out_pipe[PWRITE]);
    }
}

// If the finished flag is 1, does nothing. Otherwise, updates the
// state of cmd.  Uses waitpid() and the pid field of command to wait
// selectively for the given process. Passes block (one of DOBLOCK or
// NOBLOCK) to waitpid() to cause either non-blocking or blocking
// waits.  Uses the macro WIFEXITED to check the returned status for
// whether the command has exited. If so, sets the finished field to 1
// and sets the cmd->status field to the exit status of the cmd using
// the WEXITSTATUS macro. Calls cmd_fetch_output() to fill up the
// output buffer for later printing.
//
// When a command finishes (the first time), prints a status update
// message of the form
//
// @!!! ls[#17331]: EXIT(0)
//
// which includes the command name, PID, and exit status.
void cmd_update_state(cmd_t *cmd, int block) {
    if (cmd->finished != 1) {                                 // only update if not finished
        int status;
        pid_t pid = waitpid(cmd->pid, &status, block);        // call waitpid() on child pid
        int child_pid = cmd->pid;

        if (pid == child_pid && WIFEXITED(status) != 0) {
          if (WIFEXITED(status) != 0) {
            cmd->status = WEXITSTATUS(status);              // set status to WEXITSTATUS return val
            snprintf(cmd->str_status, STATUS_LEN, "EXIT(%d)", cmd->status);
            cmd->finished = 1;                              // set finished to 1, since it waits for finish
            printf("@!!! %s[#%d]: %s\n", cmd->name, cmd->pid, cmd->str_status);

            cmd_fetch_output(cmd);
          }
        }
    }
}


char *read_all(int fd, int *nread) {
    int cur_pos = 0;
    int max_buf_size = BUFSIZE;
    char* buffer = malloc(max_buf_size * sizeof(char));

    while (1) {
        if (cur_pos >= max_buf_size) {
            max_buf_size = max_buf_size * 2;
            buffer = realloc(buffer, max_buf_size);

            if (buffer == NULL) {
                printf("could not reallocate/expand buffer in read_all\n");
                exit(1);
            }
        }

        int max_read = max_buf_size - cur_pos;
        int num_read = read(fd, buffer + cur_pos, max_read);

        if (num_read == 0) {
            break;
        } else if (num_read == -1) {
            perror("read failed");
            exit(1);
        }

        cur_pos += num_read;
    }

    *nread = cur_pos;

    // double check that cur_pos + 1 is allocated
    if (cur_pos + 1 >= max_buf_size) {
        buffer = realloc(buffer, max_buf_size+1);
        if (buffer == NULL) {
            exit(1);
        }
    }

    // set null termination character
    buffer[cur_pos] = '\0';
    return buffer;
}
// Reads all input from the open file descriptor fd. Stores the
// results in a dynamically allocated buffer which may need to grow as
// more data is read.  Uses an efficient growth scheme such as
// doubling the size of the buffer when additional space is
// needed. Uses realloc() for resizing.  When no data is left in fd,
// sets the integer pointed to by nread to the number of bytes read
// and return a pointer to the allocated buffer. Ensures the return
// string is null-terminated. Does not call close() on the fd as this
// is done elsewhere.

void cmd_fetch_output(cmd_t *cmd) {
    if (cmd->finished == 0) {
        printf("%s[#%d] not finished yet", cmd->name, cmd->pid);
    } else {
        cmd->output = read_all(cmd->out_pipe[PREAD], &cmd->output_size);
        close(cmd->out_pipe[PREAD]);
    }
}
// If cmd->finished is zero, prints an error message with the format
//
// ls[#12341] not finished yet
//
// Otherwise retrieves output from the cmd->out_pipe and fills
// cmd->output setting cmd->output_size to number of bytes in
// output. Makes use of read_all() to efficiently capture
// output. Closes the pipe associated with the command after reading
// all input.

void cmd_print_output(cmd_t *cmd) {
    if (cmd->output != NULL) {
        write(STDOUT_FILENO, cmd->output, cmd->output_size);
    } else {
        printf("%s[#%d] : output not ready\n", cmd->name, cmd->pid);
    }
}
// Prints the output of the cmd contained in the output field if it is
// non-null. Prints the error message
//
// ls[#17251] : output not ready
//
// if output is NULL. The message includes the command name and PID.
