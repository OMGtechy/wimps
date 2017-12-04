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
#include <fcntl.h>
#include <linux/limits.h>
#include <stdatomic.h>

#include "exit_codes.h"

// use to prevent multiple samples getting written at the same time
atomic_flag wimps_sigprof_active = ATOMIC_FLAG_INIT;

// should be set by wimps_setup
int wimps_trace_fd;

// should be set by glibc
extern const char* program_invocation_short_name;

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
    if(atomic_flag_test_and_set(&wimps_sigprof_active)) {
        // there's a handler running already; drop the sample
        return;
    }

    void* trace[1000] = { NULL };
    const size_t elementSize = sizeof(trace[0]);
    const size_t size = sizeof(trace) / elementSize;

    // According to http://man7.org/linux/man-pages/man3/backtrace.3.html,
    // backtrace is safe to call from a signal hander, but loading libgcc (where it lives) isn't.
    // To get around this, we force the library to load in wimps_setup, which runs before this.
    const int addressCount = backtrace(&trace[0], size);

    // TODO: what to do if this fails?
    //       writing an error message isn't enough...and might also fail
    if(! (wimps_write(wimps_trace_fd, trace, addressCount * elementSize) && wimps_write(wimps_trace_fd, "\n", 1))) {
        const char* const failedWriteMessage = "WIMPS | ERR | Could not write to trace file";
        wimps_write(STDERR_FILENO, failedWriteMessage, strlen(failedWriteMessage));
    }

    atomic_flag_clear(&wimps_sigprof_active);
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

int wimps_create_trace_file() {
    const int badFd = -1;

    char buffer[PATH_MAX] = { '\0' };
    snprintf(buffer, PATH_MAX, "_wimps_trace_v1_pid%d_time%.f_%s_", getpid(), difftime(time(NULL), (time_t) 0), program_invocation_short_name);

    const int flags = O_WRONLY // write only access
                    | O_APPEND // append on write
                    | O_CREAT  // create a new file
                    | O_EXCL;  // error if the file exists


    const mode_t mode = S_IRUSR  // user read permissions
                      | S_IWUSR; // user write permissions

    int fd = open(buffer, flags, mode);

    if(fd == badFd) {
        // something went wrong, don't bother writing to it...
        return badFd;
    }

    // write out a header showing that it's a wimps trace file,
    // this is useful in case folk rename / move the trace files
    if(! (wimps_write(fd, buffer, strlen(buffer)) && wimps_write(fd, "\n", 1))) {
        // closing the file might change the errno, so we save and restore it
        const int err = errno;
        close(fd);
        errno = err;
        return badFd;
    }

    return fd;
}

// TODO: this should probably be refactored out of this file
__attribute__((noreturn))
void wimps_report_fatal_error(const int exitCode, const char* const format, ...) {
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

    wimps_trace_fd = wimps_create_trace_file();
    if(wimps_trace_fd == -1) {
        wimps_report_fatal_error(WIMPS_CREATE_TRACE_FILE_FAILED, "WIMPS | ERR | Could not create trace file\n");
    }

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

