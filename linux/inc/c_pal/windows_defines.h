// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef WINDOWS_DEFINES_H
#define WINDOWS_DEFINES_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void *HANDLE;
typedef void *LPSECURITY_ATTRIBUTES;

#define GENERIC_READ                    0x80000000
#define GENERIC_WRITE                   0x40000000
#define GENERIC_ALL                     0x80000000 | 0x40000000

#define FILE_SHARE_READ                 0x00000001
#define FILE_SHARE_WRITE                0x00000002
#define FILE_SHARE_DELETE               0x00000004

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000

#define CREATE_NEW                      1
#define CREATE_ALWAYS                   2
#define OPEN_EXISTING                   3
#define OPEN_ALWAYS                     4
#define TRUNCATE_EXISTING               5

#define INVALID_FILE_SIZE               ((uint32_t)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER        ((uint32_t)-1)
#define INVALID_FILE_ATTRIBUTES         ((uint32_t)-1)

#define INVALID_HANDLE_VALUE            ((HANDLE)(int64_t)-1)

#ifdef __cplusplus
}
#endif

#endif // WINDOWS_DEFINES_H