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

#include "wimps_read.h"

#include <unistd.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

bool wimps_read(const int fd, void* out, ssize_t bytes) {
    while(bytes > 0) {
        const ssize_t readBytes = read(fd, out, bytes);

        if(readBytes == -1) {
            // something went wrong!
            return false;
        }

        out += readBytes;
        bytes -= readBytes;
    }

    return true;
}

ErrorCode wimps_read_trace(const int fd, wimps_trace* const out) {
    if(out == NULL) {
        return WIMPS_ERROR_NULL_ARG;
    }

    if(fd == -1) {
        return WIMPS_ERROR_BAD_FILE;
    }

    char buffer[PATH_MAX] = { '\0' };

    {
        char* bufferNext = buffer;
        const size_t bufferSize = sizeof(buffer) / sizeof(buffer[0]);
        size_t bufferSizeLeft = bufferSize;

        // TODO: this should probably go into a wimps_readline or something,
        //       or find some standard C function that takes an fd / convert it to FILE*
        while(true) {
            // reading 1 byte at a time, like a proper tard
            if(! wimps_read(fd, bufferNext, 1)) {
                return WIMPS_ERROR_READ_FAILED;
            }

            if(*bufferNext == '\n') {
                break;
            }

            bufferNext += 1;
            bufferSizeLeft -= 1;
        }
    }

    if(strncmp(buffer, wimps_trace_marker_v1, wimps_trace_marker_v1_strlen) != 0) {
        return WIMPS_ERROR_UNKNOWN_FORMAT;
    }

    // TODO: read in samples

    return WIMPS_ERROR_NONE;
}

int main(int argc, char** argv) {
    if(argv[1] == NULL) {
        fprintf(stderr, "Please pass the name of the wimps trace file you want to read\n");
        return WIMPS_ERROR_NO_ARGS;
    }

    int fd = open(argv[1], O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Could not open trace file\n");
        return WIMPS_ERROR_READ_FAILED;
    }

    wimps_trace trace;
    ErrorCode error = wimps_read_trace(fd, &trace);

    if(error != WIMPS_ERROR_NONE) {
        const char* errorMessage = "Unknown error";
        // TODO: this should go in error_code or something
        switch(error) {
        case WIMPS_ERROR_READ_FAILED: errorMessage = "Read failed"; break;
        case WIMPS_ERROR_UNKNOWN_FORMAT: errorMessage = "Unknown format"; break;
        default: break; // remove this to get all the errors :D
        }
        fprintf(stderr, "%s\n", errorMessage);
    }

    close(fd);
}

