// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file threadapi.h
 *    @brief     This module implements support for creating new threads,
 *             terminating threads and sleeping threads.
 */

#ifndef THREADAPI_H
#define THREADAPI_H

#include <stdint.h>

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int(*THREAD_START_FUNC)(void *);

#define THREADAPI_RESULT_VALUES \
    THREADAPI_OK,               \
    THREADAPI_INVALID_ARG,      \
    THREADAPI_NO_MEMORY,        \
    THREADAPI_ERROR

/** @brief Enumeration specifying the possible return values for the APIs in
 *           this module.
 */
MU_DEFINE_ENUM(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

typedef void* THREAD_HANDLE;

/**
 * @brief    Creates a thread with the entry point specified by the @p func
 *             argument.
 *
 * @param   threadHandle    The handle to the new thread is returned in this
 *                             pointer.
 * @param   func            A function pointer that indicates the entry point
 *                             to the new thread.
 * @param   arg                A void pointer that must be passed to the function
 *                             pointed to by @p func.
 *
 * @return    @c THREADAPI_OK if the API call is successful or an error
 *             code in case it fails.
 */
MOCKABLE_FUNCTION(, THREADAPI_RESULT, ThreadAPI_Create, THREAD_HANDLE*, threadHandle, THREAD_START_FUNC, func, void*, arg);

/**
 * @brief    Blocks the calling thread by waiting on the thread identified by
 *             the @p threadHandle argument to complete.
 *
 * @param    threadHandle    The handle of the thread to wait for completion.
 * @param    res             The result returned by the thread
 *
 *            When the @p threadHandle thread completes, all resources associated
 *            with the thread must be released and the thread handle will no
 *            longer be valid.
 *
 * @return    @c THREADAPI_OK if the API call is successful or an error
 *             code in case it fails.
 */
MOCKABLE_FUNCTION(, THREADAPI_RESULT, ThreadAPI_Join, THREAD_HANDLE, threadHandle, int*, res);

/**
 * @brief    Sleeps the current thread for the given number of milliseconds.
 *
 * @param    milliseconds    The number of milliseconds to sleep.
 */
MOCKABLE_FUNCTION(, void, ThreadAPI_Sleep, unsigned int, milliseconds);

/**
 * @brief    Gets the current thread Id
 *
 * @return    @c an uint32_t value of the current thread Id
 */
MOCKABLE_FUNCTION(, uint32_t, ThreadAPI_GetCurrentThreadId);

#ifdef __cplusplus
}
#endif

#endif /* THREADAPI_H */
