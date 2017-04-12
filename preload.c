#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "exit_codes.h"

void wimps_sigprof_handler() {
    printf("WIMPS | INF | SIGPROF handler called\n");
}

void wimps_print_errno() {
    fprintf(stderr, "WIMPS | ERR | errno: %d (%s)\n", errno, strerror(errno));
}

__attribute__((constructor))
void wimps_setup() {
    {
        const void* const ret = signal(SIGPROF, &wimps_sigprof_handler);

        if(ret == SIG_ERR) {
            fprintf(stderr, "WIMPS | ERR | Could not set signal handler\n");
            exit(WIMPS_SIGNAL_FAILED);
        }

        printf("WIMPS | INF | Signal handler set\n");
    }

    struct sigevent signalEvent;
    signalEvent.sigev_notify = SIGEV_SIGNAL;
    signalEvent.sigev_signo = SIGPROF;

    static timer_t timerID;

    {
        const int ret = timer_create(CLOCK_MONOTONIC, &signalEvent, &timerID);

        if(ret != 0) {
            fprintf(stderr, "WIMPS | ERR | Could not create timer\n");
            wimps_print_errno();
            exit(WIMPS_TIMER_CREATE_FAILED);
        }

        printf("WIMPS | INF | Timer created\n");
    }

    struct itimerspec timerSpec;

    timerSpec.it_interval.tv_sec = 1;
    timerSpec.it_interval.tv_nsec = 0;
    timerSpec.it_value = timerSpec.it_interval;

    {
        const int ret = timer_settime(timerID, 0, &timerSpec, NULL);

        if(ret != 0) {
            fprintf(stderr, "WIMPS | ERR | Could not start timer\n");
            wimps_print_errno();
            exit(WIMPS_TIMER_SET_TIME_FAILED);
        }

        printf("WIMPS | INF | Timer started\n");
    }
}

