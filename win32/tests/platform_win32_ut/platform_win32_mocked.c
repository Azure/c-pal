// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "winsock2.h"

#define WSAStartup mocked_WSAStartup
#define WSACleanup mocked_WSACleanup

extern int WSAStartup(
    WORD      wVersionRequested,
    LPWSADATA lpWSAData
);

extern int WSACleanup(void);

#include "../../src/platform_win32.c"
