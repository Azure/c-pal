// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "winsock2.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/platform.h"

#include "c_logging/logger.h"

int platform_init(void)
{
    int result;

    /*Codes_SRS_PLATFORM_WIN32_88_001: [ platform_init shall call WSAStartup to initialize the Windows Sockets library with version 2.2. ]*/
    WSADATA wsaData;
    int error_code = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error_code != 0)
    {
        /*Codes_SRS_PLATFORM_WIN32_88_002: [ If WSAStartup fails, platform_init shall return a non-zero value. ]*/
        LogError("WSAStartup failed: 0x%x", error_code);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_PLATFORM_WIN32_88_003: [ If WSAStartup succeeds, platform_init shall return 0. ]*/
        result = 0;
    }
    return result;
}

void platform_deinit(void)
{
    /*Codes_SRS_PLATFORM_WIN32_88_004: [ platform_deinit shall call WSACleanup to clean up the Windows Sockets library. ]*/
    (void)WSACleanup();
}
