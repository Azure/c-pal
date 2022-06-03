// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include "windows.h"

#include "c_logging/xlogging.h"

#include "c_pal/sysinfo.h"

uint32_t sysinfo_get_processor_count(void)
{
    /* Maybe as an improvement we should use GetLogicalProcessorInformation (https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation) */
    /* Someday ... */

    uint32_t result;
    /* Codes_SRS_SYSINFO_01_001: [ sysinfo_get_processor_count shall obtain the processor count as reported by the operating system. ]*/
    /* Codes_SRS_SYSINFO_WIN32_43_001: [ sysinfo_get_processor_count shall call GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) to obtain the number of processors. ]*/
    uint32_t processor_count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);

    if (processor_count == 0)
    {
        /* Codes_SRS_SYSINFO_WIN32_43_003: [ If there are any failures, sysinfo_get_processor_count shall fail and return zero. ]*/
        LogLastError("failure in GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);");
        result = 0;
    }
    else
    {
        LogInfo("Detected %" PRIu32 " processors", processor_count);
        /* Codes_SRS_SYSINFO_WIN32_43_002: [ sysinfo_get_processor_count shall return the processor count as returned by GetActiveProcessorCount. ]*/
        result = processor_count;
    }
    return result;
}
