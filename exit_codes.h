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

enum ExitCode {
    WIMPS_SUCCESS = 0,
    WIMPS_FORK_FAILED,
    WIMPS_PTRACE_FAILED,
    WIMPS_EXEC_FAILED,
    WIMPS_MALLOC_FAILED,
    WIMPS_GETCWD_FAILED,
    WIMPS_SIGNAL_FAILED,
    WIMPS_TIMER_CREATE_FAILED,
    WIMPS_TIMER_SET_TIME_FAILED,
    WIMPS_NO_ARGS,
    WIMPS_CREATE_TRACE_FILE_FAILED
};

