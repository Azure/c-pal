// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>                       // for NULL

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "macro_utils/macro_utils.h"      // for MU_FAILURE

#include "c_logging/logger.h"

#include "c_pal/completion_port_linux.h"
#include "c_pal/platform.h"
#include "c_pal/platform_linux.h"

COMPLETION_PORT_HANDLE g_completion_port = NULL;

static int warmup_getaddrinfo(void)
{
    int result;
    struct addrinfo* addrInfo = NULL;
    struct addrinfo addrHint = { 0 };

    addrHint.ai_family = AF_UNSPEC;
    addrHint.ai_socktype = SOCK_STREAM;
    addrHint.ai_flags = AI_PASSIVE;
    
    // The below getaddrinfo call is used to avoid initialization races in getaddrinfo (nss)
    // If no warmup is done, Helgrind will complain in various int tests

    /* Codes_SRS_PLATFORM_LINUX_01_001: [ Otherwise, platform_init shall call getaddrinfo for localhost and port 4242. ]*/
    if (getaddrinfo("localhost", "4242", &addrHint, &addrInfo) != 0)
    {
        // Codes_SRS_PLATFORM_LINUX_01_002: [ If any error occurs, platform_init shall return a non-zero value. ]
        LogError("Failure calling getaddrinfo");
        result = MU_FAILURE;
    }
    else
    {
        freeaddrinfo(addrInfo);

        result = 0;
    }

    return result;
}

int platform_init(void)
{
    int result;

    if (g_completion_port == NULL)
    {
        if (warmup_getaddrinfo() != 0)
        {
            LogError("Failure calling warmup_getaddrinfo");
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SRS_PLATFORM_LINUX_11_001: [ platform_init shall call completion_port_create. ]
            g_completion_port = completion_port_create();
            if (g_completion_port == NULL)
            {
                // Codes_SRS_PLATFORM_LINUX_01_002: [ If any error occurs, platform_init shall return a non-zero value. ]
                LogError("Failure calling completion_port_create");
                result = MU_FAILURE;
            }
            else
            {
                // Codes_SRS_PLATFORM_LINUX_11_002: [ platform_init shall succeed and return zero. ]
                result = 0;
            }
        }
    }
    else
    {
        // Codes_SRS_PLATFORM_LINUX_11_007: [ If the completion port object is non-NULL, platform_init shall return zero. ]
        result = 0;
    }
    return result;
}

void platform_deinit(void)
{
    // Codes_SRS_PLATFORM_LINUX_11_008: [ If the completion port object is non-NULL, platform_deinit shall do nothing. ]
    if (g_completion_port != NULL)
    {
        // Codes_SRS_PLATFORM_LINUX_11_004: [ If the completion port object is non-NULL, platform_deinit shall decrement whose reference by calling completion_port_dec_ref. ]
        completion_port_dec_ref(g_completion_port);
        g_completion_port = NULL;
    }
}

COMPLETION_PORT_HANDLE platform_get_completion_port(void)
{
    if (g_completion_port != NULL)
    {
        // Codes_SRS_PLATFORM_LINUX_11_005: [ If the completion object is not NULL, platform_get_completion_port shall increment the reference count of the COMPLETION_PORT_HANDLE object by calling completion_port_inc_ref. ]
        completion_port_inc_ref(g_completion_port);
    }
    // Codes_SRS_PLATFORM_LINUX_11_006: [ platform_get_completion_port shall return the completion object. ]
    return g_completion_port;
}
