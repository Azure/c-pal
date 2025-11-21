// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
 #include <fcntl.h> // IWYU pragma: keep
 #include <unistd.h> // IWYU pragma: keep

#define open        mocked_open
#define close       mocked_close

int mocked_open(const char *pathname, int flags);
int mocked_close(int fd);

#include "../../src/file_util_linux.c"
