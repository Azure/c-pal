// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "winsock2.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/platform.h"

#include "c_logging/logger.h"

int platform_init(void)
{
    int result;

    WSADATA wsaData;
    int error_code = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error_code != 0)
    {
        LogError("WSAStartup failed: 0x%x", error_code);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

void platform_deinit(void)
{
    (void)WSACleanup();
}
