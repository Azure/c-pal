// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include <unistd.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"
#include "c_pal/sysinfo.h"

uint32_t sysinfo_get_processor_count(void)
{
    /* Codes_SRS_SYSINFO_01_001: [ sysinfo_get_processor_count shall obtain the processor count as reported by the operating system. ]*/
    /* Codes_SRS_SYSINFO_LINUX_01_001: [ sysinfo_get_processor_count shall call sysconf with SC_NPROCESSORS_ONLN to obtain the number of configured processors. ]*/
    uint32_t result;
    long sysconf_result = sysconf(_SC_NPROCESSORS_ONLN);
    if (sysconf_result < 0)
    {
        /* Codes_SRS_SYSINFO_01_002: [ If any error occurs, `sysinfo_get_processor_count` shall return 0. ]*/
        /* Codes_SRS_SYSINFO_LINUX_01_002: [ If any error occurs, `sysinfo_get_processor_count` shall return 0. ]*/
        LogError("sysconf(_SC_NPROCESSORS_ONLN) failed with %ld", sysconf_result);
        result = 0;
    }
    else
    {
        /* Codes_SRS_SYSINFO_LINUX_01_003: [ If sysconf returns a number bigger than UINT32_MAX, sysinfo_get_processor_count shall fail and return 0. ]*/
        if (sysconf_result > UINT32_MAX)
        {
            LogError("sysconf(_SC_NPROCESSORS_ONLN) returned %ld, wow that's a lot of processors!", sysconf_result);
            result = 0;
        }
        else
        {
            result = (uint32_t)sysconf_result;
            LogInfo("Detected %" PRIu32 " processors", result);
        }
    }

    return result;
}
