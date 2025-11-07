// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdio.h>                 // for FILE, _pclose, _popen, NULL
#include <errno.h>                 // for errno

// Tell IWYU to keep macro_utils.h because we have MU_FAILURE (even though we say that can come from umock_c_prod.h)
#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "c_logging/logger.h"
#include "c_pal/pipe.h"

FILE* pipe_popen(const char* command)
{
    FILE* result;

    /*Codes_SRS_PIPE_42_001: [ pipe_popen shall execute the command command and pipe its output to the returned FILE*. ]*/
    /*Codes_SRS_PIPE_42_002: [ If any error occurs then pipe_popen shall fail and return NULL. ]*/
    /*Codes_SRS_WIN32_PIPE_42_001: [ pipe_popen shall call _popen with command and "rt" as type. ]*/
    result = _popen(command, "rt");

    if (result == NULL)
    {
        LogError("_popen failed with %i", errno);
    }

    /*Codes_SRS_WIN32_PIPE_42_002: [ pipe_popen shall return the result of _popen. ]*/
    return result;
}

int pipe_pclose(FILE* stream, int* exit_code)
{
    int result;

    if (exit_code == NULL)
    {
        /*Codes_SRS_WIN32_PIPE_42_007: [ If exit_code is NULL then pipe_pclose shall fail and return a non-zero value. ]*/
        LogError("Invalid args: FILE* stream=%p, int* exit_code=%p",
            stream, exit_code);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_PIPE_42_003: [ pipe_pclose shall close the pipe stream. ]*/
        /*Codes_SRS_WIN32_PIPE_42_003: [ pipe_pclose shall call _pclose with stream. ]*/
        int return_value = _pclose(stream);

        if (return_value == -1)
        {
            /*Codes_SRS_PIPE_42_004: [ If any error occurs then pipe_pclose shall fail and return a non-zero value. ]*/
            /*Codes_SRS_WIN32_PIPE_42_004: [ pipe_pclose shall return a non-zero value if the return value of _pclose is -1. ]*/
            LogError("_pclose failed with %i", errno);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_PIPE_42_005: [ pipe_pclose shall store the result of the executed command in exit_code. ]*/
            /*Codes_SRS_WIN32_PIPE_42_006: [ pipe_pclose shall store the return value of _pclose in exit_code. ]*/
            *exit_code = return_value;

            /*Codes_SRS_PIPE_42_006: [ pipe_pclose shall succeed and return 0. ]*/
            /*Codes_SRS_WIN32_PIPE_42_005: [ Otherwise, pipe_pclose shall return 0. ]*/
            result = 0;
        }
    }

    return result;
}
