// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include "windows.h"

#include "c_logging/xlogging.h"

#include "c_pal/sysinfo.h"

uint32_t sysinfo_get_processor_count(void)
{
    /* Maybe as an improvement we should use GetLogicalProcessorInformation (https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation) */
    /* Someday ... */

    /* Codes_SRS_SYSINFO_01_001: [ sysinfo_get_processor_count shall obtain the processor count as reported by the operating system. ]*/
    /* Codes_SRS_SYSINFO_WIN32_43_001: [ sysinfo_get_processor_count shall call GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) to obtain the number of processors. ]*/
    /* Codes_SRS_SYSINFO_WIN32_43_002: [ sysinfo_get_processor_count shall return the processor count as returned by GetActiveProcessorCount. ]*/
    uint32_t result = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    LogInfo("Detected %" PRIu32 " processors", result);
    return result;
}
