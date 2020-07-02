// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef FILE_INTERNAL_H
#define FILE_INTERNAL_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"
#include "azure_macro_utils/macro_utils.h"
#include "constbuffer.h"
#include "file.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FILE_WRITE_DATA_TAG
{
    FILE_WRITE_CB user_callback;
    void* user_context;
    CONSTBUFFER_HANDLE source;
}FILE_WRITE_DATA;

typedef struct FILE_READ_DATA_TAG
{
    FILE_READ_CB user_callback;
    void* user_context;
    uint32_t size;
    unsigned char* destination;
}FILE_READ_DATA;

typedef union FILE_IO_DATA_TAG
{
    FILE_READ_DATA read_data;
    FILE_WRITE_DATA write_data;
}FILE_IO_DATA;

#define FILE_ASYNC_OPERATION_VALUES \
    FILE_ASYNC_WRITE, \
    FILE_ASYNC_READ

MU_DEFINE_ENUM(FILE_ASYNC_OPERATION, FILE_ASYNC_OPERATION_VALUES)

#ifdef __cplusplus
}
#endif
#endif