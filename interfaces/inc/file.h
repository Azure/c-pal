// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SYNC_H
#define SYNC_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#include "umock_c/umock_c_prod.h"
#include "constbuffer.h"
#ifdef __cplusplus
extern "C" {
#endif

#define FILE_DATA_RESULT_VALUES \
    FILE_DATA_FAILURE, \
    FILE_DATA_INVALID, \
    FILE_DATA_OK

MU_DEFINE_ENUM(FILE_DATA_RESULT, FILE_DATA_RESULT_VALUES);

typedef void *FILE_HANDLE;
typedef void(*FILE_WRITE_CB)(void* userContext, bool isSuccess);
typedef void(*FILE_READ_CB)(void* userContext, FILE_DATA_RESULT result, CONSTBUFFER_HANDLE content);

MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_HANDLE, file_open, const char*, fullFileName, int64_t, desire_file_size)(0, MU_FAILURE);
MOCKABLE_FUNCTION(, void, file_close, FILE_HANDLE, handle);

MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_write_async, FILE_HANDLE, handle, CONSTBUFFER_HANDLE, source, int64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_read_async, FILE_HANDLE, handle, uint32_t, size, int64_t, position, FILE_READ_CB, user_callback, void*, user_context)(0, MU_FAILURE);

#ifdef __cplusplus
}
#endif
#endif