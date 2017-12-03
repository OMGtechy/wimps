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

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <stdbool.h>
#include <stdarg.h>

#include "exit_codes.h"

bool wimps_write(int fd, const void* buffer, ssize_t size) {
    while(size > 0) {
        ssize_t written = write(fd, buffer, size);

        if(written == -1) {
            // something went wrong,
            // but don't close the file as the
            // caller might want something different
            return false;
        }

        buffer += written;
        size -= written;
    }

    return true;
}

void wimps_sigprof_handler() {
    void* trace[1000] = { NULL };
    const size_t elementSize = sizeof(trace[0]);
    const size_t size = sizeof(trace) / elementSize;

    // According to http://man7.org/linux/man-pages/man3/backtrace.3.html,
    // backtrace is safe to call from a signal hander, but loading libgcc (where it lives) isn't.
    // To get around this, we force the library to load in wimps_setup, which runs before this.
    const int addressCount = backtrace(&trace[0], size);

    // TODO: write out to a file, it's meaningless on stdout...
    wimps_write(0, trace, addressCount * elementSize);
}

void wimps_force_libgcc_load() {
    void* dummy;
    backtrace(&dummy, 1);
}

bool wimps_set_signal_handler(void (*handler)()) {
    return signal(SIGPROF, handler) != SIG_ERR;
}

bool wimps_create_timer(timer_t* const outTimer) {
    struct sigevent signalEvent;
    signalEvent.sigev_notify = SIGEV_SIGNAL;
    signalEvent.sigev_signo = SIGPROF;

    return timer_create(CLOCK_MONOTONIC, &signalEvent, outTimer) == 0;
}

bool wimps_start_timer(timer_t timer) {
    struct itimerspec timerSpec;

    timerSpec.it_interval.tv_sec = 1;
    timerSpec.it_interval.tv_nsec = 0;
    timerSpec.it_value = timerSpec.it_interval;

    return timer_settime(timer, 0, &timerSpec, NULL) == 0;
}

// TODO: this should probably be refactored out of this file
__attribute__((noreturn))
void wimps_report_fatal_error(int exitCode, const char* const format, ...) {
    fprintf(stderr, "WIMPS | ERR | errno: %d (%s)\n", errno, strerror(errno));

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "WIMPS | ERR | exiting with code %d\n", exitCode);
    exit(exitCode);
}

__attribute__((constructor))
void wimps_setup() {
    // see wimps_sigprof_handler for why this is needed
    wimps_force_libgcc_load();

    if(! wimps_set_signal_handler(&wimps_sigprof_handler)) {
        wimps_report_fatal_error(WIMPS_SIGNAL_FAILED, "WIMPS | ERR | Could not set signal handler\n");
    }

    timer_t timer;
    if(! wimps_create_timer(&timer)) {
        wimps_report_fatal_error(WIMPS_TIMER_CREATE_FAILED, "WIMPS | ERR | errno: %d (%s)\n", errno, strerror(errno));
    }

    if(! wimps_start_timer(timer)) {
        wimps_report_fatal_error(WIMPS_TIMER_SET_TIME_FAILED, "WIMPS | ERR | Could not start timer\n");
    }
}

