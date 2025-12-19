// Copyright (c) Microsoft. All rights reserved.

#define sysconf mocked_sysconf

long mocked_sysconf(int name);

#include "../../src/sysinfo_linux.c"
