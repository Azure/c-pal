// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef error_handling_linux_H
#define error_handling_linux_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#endif
#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32 
#include "windows.h"
#else

MOCKABLE_FUNCTION(, void, error_handling_linux_set_last_error, volatile_atomic int64_t, err_code);
MOCKABLE_FUNCTION(, uint64_t, error_handling_linux_get_last_error);

#ifdef __cplusplus
}
#endif

#endif // error_handling_linux_H
