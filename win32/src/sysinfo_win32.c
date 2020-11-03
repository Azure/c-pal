// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include "windows.h"

#include "c_logging/xlogging.h"

#include "c_pal/sysinfo.h"

uint32_t sysinfo_get_processor_count(void)
{
    SYSTEM_INFO system_info;
    uint32_t result;

    /* Maybe as an improvement we should use GetLogicalProcessorInformation (https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation) */
    /* Someday ... */

    /* Codes_SRS_SYSINFO_01_001: [ sysinfo_get_processor_count shall obtain the processor count as reported by the operating system. ]*/
    /* Codes_SRS_SYSINFO_WIN32_01_001: [ sysinfo_get_processor_count shall call GetSystemInfo to obtain the system information. ]*/
    GetSystemInfo(&system_info);

    /* Codes_SRS_SYSINFO_WIN32_01_002: [ sysinfo_get_processor_count shall return the processor count as returned by GetSystemInfo. ]*/
    result = system_info.dwNumberOfProcessors;

    LogInfo("Detected %" PRIu32 " processors", result);

    return result;
}
