// Copyright(C) Microsoft Corporation.All rights reserved.

#ifndef MOCKED_WINDOWS_H
#define MOCKED_WINDOWS_H

// Define _WINDOWS_ to let mock headers know windows.h has been included
#ifndef _WINDOWS_
#define _WINDOWS_
#endif

#include <stdint.h>

typedef void *HANDLE; /*works for most of the cases, except for those sneaky handles returned by CreateFile for which INVALID_HANDLE_VALUE is "the NULL"*/

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

typedef void* LONG_PTR; /*approximatively*/

typedef void* PTP_IO;
typedef void* PTP_POOL;
typedef void* PTP_TIMER;
typedef void* PTP_WORK;
typedef void* PTP_CLEANUP_GROUP;
typedef void* PTP_CALLBACK_ENVIRON;
typedef void* PTP_CALLBACK_INSTANCE;
typedef void* PFILETIME;

typedef int64_t LONGLONG;
typedef int64_t LONG64;
typedef int32_t LONG;
typedef unsigned long ULONG;
typedef uint64_t ULONG64;
typedef uint64_t ULONGLONG;
typedef LONG HRESULT;
typedef unsigned long DWORD;
typedef void VOID;
typedef void* PVOID;
typedef size_t SIZE_T;
typedef long BOOL;

#define CALLBACK

typedef VOID (CALLBACK *PTP_TIMER_CALLBACK)(PTP_CALLBACK_INSTANCE, PVOID, PTP_TIMER);
typedef VOID (CALLBACK *PTP_WORK_CALLBACK)(PTP_CALLBACK_INSTANCE, PVOID, PTP_WORK);
typedef VOID (CALLBACK *PTP_CLEANUP_GROUP_CANCEL_CALLBACK)(PVOID, PVOID);

typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    };
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

#define FALSE 0
#define TRUE 1
#define INFINITE 0xFFFFFFFF

    LONGLONG InterlockedAdd64(
        LONGLONG volatile  * Addend,
        LONGLONG          Value
    );

    LONGLONG InterlockedCompareExchange64(
       LONG64 volatile *Destination,
       LONG64          Exchange,
       LONG64          Comparand
    );

    BOOL WaitOnAddress(
        volatile VOID * Address,
        PVOID CompareAddress,
        SIZE_T AddressSize,
        DWORD dwMilliseconds
        );

    LONG InterlockedExchange(
        LONG volatile* Target,
        LONG          Value
    );

    void WakeByAddressSingle(
        PVOID Address
    );

    void WakeByAddressAll(
        PVOID Address
    );

    LONG InterlockedAdd(
        LONG volatile *Addend,
        LONG          Value
    );

#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800

#define FormatMessage(...) 0
#define GetLastError() 0

#endif/*MOCKED_WINDOWS_H*/
