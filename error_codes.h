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

typedef enum _ErrorCode {
    WIMPS_ERROR_NONE = 0,
    WIMPS_ERROR_FORK_FAILED,
    WIMPS_ERROR_PTRACE_FAILED,
    WIMPS_ERROR_EXEC_FAILED,
    WIMPS_ERROR_MALLOC_FAILED,
    WIMPS_ERROR_GETCWD_FAILED,
    WIMPS_ERROR_SIGNAL_FAILED,
    WIMPS_ERROR_TIMER_CREATE_FAILED,
    WIMPS_ERROR_TIMER_SET_TIME_FAILED,
    WIMPS_ERROR_NO_ARGS,
    WIMPS_ERROR_CREATE_TRACE_FILE_FAILED,
    WIMPS_ERROR_NULL_ARG,
    WIMPS_ERROR_BAD_FILE,
    WIMPS_ERROR_READ_FAILED,
    WIMPS_ERROR_ASSUMPTION_FAILED,
    WIMPS_ERROR_UNKNOWN_FORMAT,
    WIMPS_ERROR_EOF,
    WIMPS_ERROR_BAD_MARKER,
    WIMPS_ERROR_REALLOC_FAILED
} ErrorCode;

