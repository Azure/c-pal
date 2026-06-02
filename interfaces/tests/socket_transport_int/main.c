// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#ifdef WIN32
#include "winsock2.h"
#include "ws2tcpip.h"
#endif

#include "c_logging/logger.h"

#include "testrunnerswitcher.h"

#ifdef WIN32
// On the first name resolution call (getaddrinfo) Winsock lazily loads a namespace provider
// DLL. That one time load performs a heap allocation inside WS2_32/ntdll that lives for the
// lifetime of the process and is only released when the DLL is unloaded during process exit.
// ctest samples the VLD leak baseline at the very start of the run (before any test, including
// the suite initialize, executes), so when that allocation happens during the first test it is
// counted as a leak even though it is not one (VLD's own end of process report confirms no real
// leak). Whether VLD attributes the block at the sampling point is timing dependent, which made
// this test flaky (observed failing on ARM64 and reproducible on x64). Warming up name
// resolution here, before the baseline is captured, folds that one time allocation into the
// baseline so the leak delta nets to zero. The namespace provider DLL loaded by the first
// getaddrinfo is observed to stay resident after WSACleanup in the configurations tested, so its
// one time loader allocation persists for the rest of the run; WSACleanup is still called here so
// that Winsock's per startup caches are released and not reported as leaks themselves.
static void warm_up_winsock_name_resolution(void)
{
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0)
    {
        ADDRINFO hints = { 0 };
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;
        hints.ai_flags = AI_CANONNAME;

        ADDRINFO* addr_info = NULL;
        // Use the same host, family, socktype and flags as the production path in
        // connect_to_client so the same namespace provider is loaded; the numeric service value
        // mirrors the ports the tests use.
        if (getaddrinfo("localhost", "4466", &hints, &addr_info) == 0)
        {
            freeaddrinfo(addr_info);
        }

        (void)WSACleanup();
    }
}
#endif

int main(int argc, char* argv[])
{
    size_t failed_test_count = 0;

    // If a command line argument is provided, use it as a test name filter
    // to run only the test case matching that name
    const char* test_name_filter = (argc > 1) ? argv[1] : NULL;

#ifdef WIN32
    warm_up_winsock_name_resolution();
#endif

    (void)logger_init();

    RUN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE, failed_test_count, test_name_filter);

    logger_deinit();

    return failed_test_count;
}
