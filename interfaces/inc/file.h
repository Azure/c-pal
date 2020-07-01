// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef FILE_H
#define FILE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"
#include "azure_macro_utils/macro_utils.h"
#include "constbuffer.h"
#include "execution_engine.h"
#ifdef __cplusplus
extern "C" {
#endif

#define FILE_WRITE_ASYNC_VALUES \
    FILE_WRITE_ASYNC_INVALID_ARGS, \
    FILE_WRITE_ASYNC_WRITE_ERROR, \
    FILE_WRITE_ASYNC_ERROR,\
    FILE_WRITE_ASYNC_OK
MU_DEFINE_ENUM(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_VALUES);

#define FILE_READ_ASYNC_VALUES \
    FILE_READ_ASYNC_INVALID_ARGS, \
    FILE_READ_ASYNC_READ_ERROR, \
    FILE_READ_ASYNC_ERROR,\
    FILE_READ_ASYNC_OK
MU_DEFINE_ENUM(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_VALUES);

typedef void *FILE_HANDLE;
typedef void(*FILE_WRITE_CB)(void* user_context, bool is_successful);
typedef void(*FILE_READ_CB)(void* user_context, bool is_successful, CONSTBUFFER_HANDLE content);


MOCKABLE_FUNCTION(, FILE_HANDLE, file_create, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name, uint64_t, desired_file_size, bool, has_manage_volume);
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);

MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, CONSTBUFFER_HANDLE, source, uint64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint32_t, size, uint64_t, position, FILE_READ_CB, user_callback, void*, user_context)(0, MU_FAILURE);

MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend_filesize, FILE_HANDLE, handle, uint64_t, desired_size, bool, has_manage_volume)(0, MU_FAILURE);
#ifdef __cplusplus
}
#endif
#endif