#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <linux/limits.h>

#include "exit_codes.h"

extern char** environ;

pid_t child_pid;
void sigint_handler() {
    printf("WIMPS | INF | Recieved signal %d (%s), forwarding to child process\n", SIGINT, strsignal(SIGINT));
    kill(child_pid, SIGINT);
}

enum ExitCode parent(pid_t child) {
    child_pid = child;
    signal(SIGINT, &sigint_handler);

    while(1) {
        int status;
        waitpid(child, &status, 0); 
        printf("WIMPS | INF | Child process status changed\n");

        if(WIFEXITED(status)) {
            printf("WIMPS | INF | Child process exited normally\n");
            return WIMPS_SUCCESS;
        }

        if(WIFSIGNALED(status)) {
            int signalNumber = WTERMSIG(status);
            printf("WIMPS | INF | Child process killed by signal %d (%s)\n", signalNumber, strsignal(signalNumber));
            return WIMPS_SUCCESS;
        }

        if(WIFSTOPPED(status)) {
            int signalNumber = WSTOPSIG(status);
            printf("WIMPS | INF | Child process stopped by signal %d (%s)\n", signalNumber, strsignal(signalNumber));

            int signalToSend;

            switch(signalNumber) {
            case SIGINT:
            case SIGPROF:
                signalToSend = signalNumber;
                break;
            default:
                signalToSend = 0;
                break;
            }

            if(signalToSend != 0) {
                printf("WIMPS | INF | Forwarding signal to child process\n");
            }

            printf("WIMPS | INF | Continuing child process\n");
            if(ptrace(PTRACE_CONT, child, 0, signalToSend) == -1) {
                fprintf(stderr, "WIMPS | ERR | Failed to continue child process\n");
                return WIMPS_PTRACE_FAILED;
            }
        }
    }
}

enum ExitCode child(char** argv) {
    if(ptrace(PTRACE_TRACEME) == -1) {
        fprintf(stderr, "WIMPS | ERR | Could not execute PTRACE_TRACEME\n");
        return WIMPS_PTRACE_FAILED;
    }

    size_t env_size = 0;
    while(environ[env_size] != NULL) {
        env_size += 1;
    }

    // add one on for LD_PRELOAD ...
    env_size += 1;

    // add one on for NULL ...
    env_size += 1;

    char** env = malloc(env_size * sizeof(char*));

    size_t i = 0;
    for(; environ[i] != NULL; ++i) {
        env[i] = environ[i];
    }

    char path[PATH_MAX] = "LD_PRELOAD=";
    const size_t ldPreloadLength = strlen(path);
    if(getcwd(&path[ldPreloadLength], PATH_MAX - ldPreloadLength - 1) == NULL) {
        printf("WIMPS | ERR | Could not get current working directory\n");
        return WIMPS_GETCWD_FAILED;
    }

    strcat(path, "/libpreload.so");

    env[i++] = path;
    env[i] = NULL;

    if(env == NULL) {
        fprintf(stderr, "WIMPS | ERR | Could not malloc environment for exec\n");
        return WIMPS_MALLOC_FAILED;
    }

    printf("WIMPS | INF | Executing %s", argv[0]);

    for(size_t i = 1; argv[i] != NULL; ++i) {
        printf(" %s", argv[i]);
    }

    printf("\n");

    execve(argv[0], argv, env);

    // we will only get here if the exec failed
    fprintf(stderr, "WIMPS | ERR | Could not exec into target program\n");
    return WIMPS_EXEC_FAILED;
}

int main(int argc, char** argv) {
    if(argv[1] == NULL) {
        fprintf(stderr, "WIMPS | ERR | Please pass the program you want to run\n");
        return WIMPS_NO_ARGS;
    }

    const pid_t pid = fork();

    switch(pid) {
    case -1:
        fprintf(stderr, "WIMPS | ERROR | Could not create initial child process\n");
        return WIMPS_FORK_FAILED;
    case 0:
        return child(argv + 1);
    default:
        return parent(pid /* the child pid */);
    }
}

