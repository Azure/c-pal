// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

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

MOCKABLE_FUNCTION(, void, error_handling_set_last_error, atomic_uint64_t, err_code);
MOCKABLE_FUNCTION(, atomic_uint64_t, error_handling_get_last_error);

#ifdef __cplusplus
}
#endif

#endif // ERROR_HANDLING_H
