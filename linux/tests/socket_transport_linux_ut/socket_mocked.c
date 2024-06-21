// Copyright (C) Microsoft Corporation. All rights reserved.

#include "socket_mocked.h"

#include "../../src/socket_transport_linux.c"

int mocked_fcntl(int __fd, int __cmd, ...)
{
    (void)__fd;
    (void)__cmd;
    return 0;
}
