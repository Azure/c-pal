// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <inttypes.h>

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep

#include "c_logging/logger.h"
#include "c_pal/interlocked.h"

#include "c_pal/error_handling.h"

static volatile_atomic int32_t last_error_code;

/*Codes_SRS_ERROR_HANDLING_LINUX09_002: [ error_handling_linux_set_last_error shall assign a non-NULL value to last_error_code. ]*/
void error_handling_linux_set_last_error(uint32_t err_code)
{   
    SetLastError(err_code);
}

/*Codes_SRS_ERROR_HANDLING_LINUX09_005: [ On success, error_handling_linux_get_last_error shall return the value last set through set_last_error or zero ]*/
uint32_t error_handling_linux_get_last_error(void)
{

    uint32_t return_val;
    return_val = GetLastError();

    return return_val;
}
