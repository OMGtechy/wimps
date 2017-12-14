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

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "error_codes.h"

// If the original typedef is anything other than the type specified,
// this'll cause a compile time error...
//
// Think of it as a hacky type equality assertion
typedef int64_t time_t;
typedef long int64_t;

typedef struct _wimps_timespec {
    int64_t seconds;
    int64_t nanoseconds;
} wimps_timespec;

typedef struct _wimps_sample {
    wimps_timespec time;
    void* addresses;
    size_t addressCount;
} wimps_sample;

typedef struct _wimps_trace {
    wimps_sample* samples;
    size_t sampleCount;
} wimps_trace;

const char wimps_trace_marker_v1[] = "_wimps_trace_v1";
const size_t wimps_trace_marker_v1_strlen = sizeof(wimps_trace_marker_v1) / sizeof(wimps_trace_marker_v1[0]) - 1 /* null terminator */;

