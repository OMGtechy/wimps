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
#include <stdlib.h>

ErrorCode wimps_read(const int fd, void* out, ssize_t bytes) {
    while(bytes > 0) {
        const ssize_t readBytes = read(fd, out, bytes);

        if(readBytes == -1) {
            // something went wrong!
            return WIMPS_ERROR_READ_FAILED;
        }

        if(readBytes == 0) {
            return WIMPS_ERROR_EOF;
        }

        out += readBytes;
        bytes -= readBytes;
    }

    return WIMPS_ERROR_NONE;
}

ErrorCode wimps_check_marker_char(int fd, char expected) {
    char actual;
    const ErrorCode error = wimps_read(fd, &actual, 1);
    _Static_assert(sizeof(char) == 1, "Assumed sizeof(char) was 1...it's not");

    if(error != WIMPS_ERROR_NONE) {
        return error;
    }

    return actual == expected ? WIMPS_ERROR_NONE : WIMPS_ERROR_BAD_MARKER;
}

ErrorCode wimps_readline(int fd, char* buffer, const size_t bufferSize) {
    char* bufferNext = buffer;
    size_t bufferSizeLeft = bufferSize;

    while(true) {
        const ErrorCode error = wimps_read(fd, bufferNext, 1);

        if(error != WIMPS_ERROR_NONE) {
            return error;
        }

        if(*bufferNext == '\n') {
            break;
        }

        bufferNext += 1;
        bufferSizeLeft -= 1;
    }

    return WIMPS_ERROR_NONE;
}

ErrorCode wimps_read_trace(const int fd, wimps_trace* const out) {
    if(out == NULL) {
        return WIMPS_ERROR_NULL_ARG;
    }

    // we use realloc later...
    // if this isn't null, one of the following has happened:
    // (1) the caller just didn't zero the memory
    // (2) the caller is reusing it
    // we assume (1). (2) is a memory leak.
    out->samples = NULL;
    out->sampleCount = 0;

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
            const ErrorCode error = wimps_read(fd, bufferNext, 1);

            if(error != WIMPS_ERROR_NONE) {
                return error;
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

    while(true) {
        // get marker "a"
        // it's ok to fail if it's EOF (i.e. no more samples)
        {
            const ErrorCode error = wimps_check_marker_char(fd, 'a');
            if(error != WIMPS_ERROR_NONE) {
                if(error == WIMPS_ERROR_EOF) {
                    break;
                }
            }
        }

        // make room for one more sample
        // it's incremented at the end when we know the sample is at least probably valid
        // TODO: clean up memory on later failure
        {
            wimps_sample* newSamples = realloc(out->samples,
                                               (out->sampleCount + 1) * sizeof(wimps_sample));

            if(newSamples == NULL) {
                return WIMPS_ERROR_REALLOC_FAILED;
            }

            out->samples = newSamples;
        }

        wimps_sample* const currentSample = &out->samples[out->sampleCount];
        out->sampleCount += 1;

        // get the time of sample
        {
           const ErrorCode error = wimps_read(fd,
                                              &currentSample->time,
                                              sizeof(currentSample->time));

            if(error != WIMPS_ERROR_NONE) {
                return error;
            }
        }

        // get marker "b"
        {
            const ErrorCode error = wimps_check_marker_char(fd, 'b');
            if(error != WIMPS_ERROR_NONE) {
                return error;
            }
        }

        // get the symbols
        {
            for(size_t i = 0;; ++i) {
                char strbuffer[1000] = { '\0' };
                const ErrorCode error = wimps_readline(fd, strbuffer, sizeof(strbuffer) - 1);

                if(error != WIMPS_ERROR_NONE) {
                    return error;
                }

                if(strncmp(wimps_end_sample_marker, strbuffer, wimps_end_sample_marker_strlen) == 0) {
                    break;
                }

                // chop off the newline character
                strbuffer[strlen(strbuffer) - 1] = '\0';

                const char** symbols = realloc(currentSample->symbols, sizeof(char*) * (i + 1));

                if(symbols == NULL) {
                    return WIMPS_ERROR_REALLOC_FAILED;
                }

                const char* const symbol = strndup(strbuffer, sizeof(strbuffer));

                if(symbol == NULL) {
                    return WIMPS_ERROR_STRNDUP_FAILED;
                }

                currentSample->symbols = symbols;
                currentSample->symbols[i] = symbol;
                currentSample->symbolCount += 1;
            }
        }

        // get marker "c"
        {
            const ErrorCode error = wimps_check_marker_char(fd, 'c');
            if(error != WIMPS_ERROR_NONE) {
                return error;
            }
        }
    }

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
    const ErrorCode error = wimps_read_trace(fd, &trace);

    if(error != WIMPS_ERROR_NONE) {
        fprintf(stderr, "%s\n", wimps_error_string(error));
    } else {
        for(size_t i = 0; i < trace.sampleCount; ++i) {
            printf("Sample %zu\n", i);
            for(size_t j = 0; j < trace.samples[i].symbolCount; ++j) {
                printf("\t%s\n", trace.samples[i].symbols[j]);
            }
        }
    }

    // TODO: free trace memory? doesn't matter much for now since the program closes after

    close(fd);
    return error;
}

