// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <errno.h>


#include "c_logging/logger.h"

#include "c_pal/pipe.h"

FILE* pipe_popen(const char* command)
{
    FILE* result;

    /*Codes_SRS_PIPE_42_001: [ pipe_popen shall execute the command command and pipe its output to the returned FILE*. ]*/
    /*Codes_SRS_PIPE_42_002: [ If any error occurs then pipe_popen shall fail and return NULL. ]*/
    /*Codes_SRS_LINUX_PIPE_42_001: [ pipe_popen shall call popen with command and "r" as type. ]*/
    result = popen(command, "r");

    if (result == NULL)
    {
        LogError("popen failed with %i", errno);
    }

    /*Codes_SRS_LINUX_PIPE_42_002: [ pipe_popen shall return the result of popen. ]*/
    return result;
}

int pipe_pclose(FILE* stream, int* exit_code)
{
    int result;

    if (exit_code == NULL)
    {
        /*Codes_SRS_LINUX_PIPE_42_007: [ If exit_code is NULL then pipe_pclose shall fail and return a non-zero value. ]*/
        LogError("Invalid args: FILE* stream = %p, int* exit_code = %p",
            stream, exit_code);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_PIPE_42_003: [ pipe_pclose shall close the pipe stream. ]*/
        /*Codes_SRS_LINUX_PIPE_42_003: [ pipe_pclose shall call pclose with stream. ]*/
        int return_value = pclose(stream);

        if (return_value == -1)
        {
            /*Codes_SRS_PIPE_42_004: [ If any error occurs then pipe_pclose shall fail and return a non-zero value. ]*/
            /*Codes_SRS_LINUX_PIPE_42_004: [ pipe_pclose shall return a non-zero value if the return value of pclose is -1. ]*/
            LogError("pclose failed with %i", errno);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_PIPE_42_005: [ pipe_pclose shall store the result of the executed command in exit_code. ]*/
            /*Codes_SRS_LINUX_PIPE_42_006: [ pipe_pclose shall store the return value of pclose bit-shifted right by 8 to get the exit code from the command to exit_code. ]*/
            *exit_code = return_value >> 8;

            /*Codes_SRS_PIPE_42_006: [ pipe_pclose shall succeed and return 0. ]*/
            /*Codes_SRS_LINUX_PIPE_42_005: [ Otherwise, pipe_pclose shall return 0. ]*/
            result = 0;
        }
    }

    return result;
}
