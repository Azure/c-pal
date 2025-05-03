// Copyright (c) Microsoft. All rights reserved.

#ifndef PLATFORM_MOCKED_H
#define PLATFORM_MOCKED_H

#include <stddef.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "umock_c/umock_c_prod.h"

struct addrinfo;

#define getaddrinfo mocked_getaddrinfo
#define freeaddrinfo mocked_freeaddrinfo

MOCKABLE_FUNCTION(, int, mocked_getaddrinfo, const char*, pNodeName, const char*, pServiceName, const struct addrinfo*, pHints, struct addrinfo**, ppResult);
MOCKABLE_FUNCTION(, void, mocked_freeaddrinfo, struct addrinfo*, pAddrInfo);

#endif // PLATFORM_MOCKED_H
