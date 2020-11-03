// Copyright (c) Microsoft. All rights reserved.

#include <unistd.h>

#define sysconf mocked_sysconf

extern long mocked_sysconf(int name);

#include "../../src/sysinfo_linux.c"
