// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, void, error_handling_linux_set_last_error, uint32_t, err_code);
MOCKABLE_FUNCTION(, uint32_t, error_handling_linux_get_last_error);

#ifdef __cplusplus
}
#endif

#endif // error_handling_H
