// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef WINDOWS_DEFINES_H
#define WINDOWS_DEFINES_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32

#include "windows.h"

#else

#include "windows_defines_errors.h"
typedef void *HANDLE;
typedef void *LPSECURITY_ATTRIBUTES;
typedef void *LPOVERLAPPED_COMPLETION_ROUTINE;
typedef void *LPCVOID;
typedef void *PVOID;
typedef void *CALLBACK;
typedef const char* LPCSTR;
typedef void *PTP_CALLBACK_ENVIRON;
typedef void *PTP_IO;
typedef void *PTP_CALLBACK_INSTANCE;
typedef void *CALLBACK;
typedef void *ULONG_PTR;
typedef void *ULONG;
typedef void *PTP_CLEANUP_GROUP;
typedef void *PTP_POOL;
typedef void *PTP_WIN32_IO_CALLBACK;

typedef struct _OVERLAPPED {
  uint32_t* Internal;
  uint32_t* InternalHigh;
  union {
    struct {
      uint32_t Offset;
      uint32_t OffsetHigh;
    } DUMMYSTRUCTNAME;
    PVOID Pointer;
  } DUMMYUNIONNAME;
  HANDLE    hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef void (*PTP_WIN32_IO_CALLBACK_FUNC)(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID                 Context,
    PVOID                 Overlapped,
    uint64_t              IoResult,
    uint32_t             NumberOfBytesTransferred,
    PTP_IO                Io
);


#define GENERIC_READ                    (0x80000000)
#define GENERIC_WRITE                   (0x40000000)
#define GENERIC_ALL                     (0x80000000 | 0x40000000)

#define FILE_SHARE_READ                 (0x00000001)
#define FILE_SHARE_WRITE                (0x00000002)
#define FILE_SHARE_DELETE               (0x00000004)

#define FILE_FLAG_WRITE_THROUGH         (0x80000000)
#define FILE_FLAG_OVERLAPPED            (0x40000000)
#define FILE_FLAG_NO_BUFFERING          (0x20000000)
#define FILE_FLAG_RANDOM_ACCESS         (0x10000000)
#define FILE_FLAG_SEQUENTIAL_SCAN       (0x08000000)
#define FILE_FLAG_DELETE_ON_CLOSE       (0x04000000)
#define FILE_FLAG_OPEN_NO_RECALL        (0x00100000)

#define CREATE_NEW                      1
#define CREATE_ALWAYS                   2
#define OPEN_EXISTING                   3
#define OPEN_ALWAYS                     4
#define TRUNCATE_EXISTING               5

#define INVALID_FILE_SIZE               ((uint32_t)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER        ((uint32_t)-1)
#define INVALID_FILE_ATTRIBUTES         ((uint32_t)-1)

#define INVALID_HANDLE_VALUE            ((HANDLE)(int64_t)-1)

#endif //WIN32

#ifdef __cplusplus
}
#endif

#endif // WINDOWS_DEFINES_H
