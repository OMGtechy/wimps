/*
    This file is part of wimps.

    wimps is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    wimps is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with wimps.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <linux/limits.h>

#include "error_codes.h"

extern char** environ;

pid_t child_pid;
void sigint_handler() {
    kill(child_pid, SIGINT);
}

ErrorCode parent(pid_t child) {
    child_pid = child;
    signal(SIGINT, &sigint_handler);

    while(1) {
        int status;
        waitpid(child, &status, 0); 

        if(WIFEXITED(status)) {
            return WIMPS_ERROR_NONE;
        }

        if(WIFSIGNALED(status)) {
            return WIMPS_ERROR_NONE;
        }

        if(WIFSTOPPED(status)) {
            int signalNumber = WSTOPSIG(status);
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

            if(ptrace(PTRACE_CONT, child, 0, signalToSend) == -1) {
                fprintf(stderr, "WIMPS | ERR | Failed to continue child process\n");
                return WIMPS_ERROR_PTRACE_FAILED;
            }
        }
    }
}

ErrorCode child(char** argv) {
    if(ptrace(PTRACE_TRACEME) == -1) {
        fprintf(stderr, "WIMPS | ERR | Could not execute PTRACE_TRACEME\n");
        return WIMPS_ERROR_PTRACE_FAILED;
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
        return WIMPS_ERROR_GETCWD_FAILED;
    }

    strcat(path, "/libpreload.so");

    env[i++] = path;
    env[i] = NULL;

    if(env == NULL) {
        fprintf(stderr, "WIMPS | ERR | Could not malloc environment for exec\n");
        return WIMPS_ERROR_MALLOC_FAILED;
    }

    printf("WIMPS | INF | Executing %s", argv[0]);

    for(size_t i = 1; argv[i] != NULL; ++i) {
        printf(" %s", argv[i]);
    }

    printf("\n");

    execve(argv[0], argv, env);

    // we will only get here if the exec failed
    free(env);
    fprintf(stderr, "WIMPS | ERR | Could not exec into target program\n");
    return WIMPS_ERROR_EXEC_FAILED;
}

int main(int argc, char** argv) {
    if(argv[1] == NULL) {
        fprintf(stderr, "WIMPS | ERR | Please pass the program you want to run\n");
        return WIMPS_ERROR_NO_ARGS;
    }

    const pid_t pid = fork();

    switch(pid) {
    case -1:
        fprintf(stderr, "WIMPS | ERR | Could not create initial child process\n");
        return WIMPS_ERROR_FORK_FAILED;
    case 0:
        return child(argv + 1);
    default:
        return parent(pid /* the child pid */);
    }
}

