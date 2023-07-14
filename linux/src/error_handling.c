// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <inttypes.h>

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep

#include "c_logging/logger.h"

#include "c_pal/windows_defines.h"
#include "c_pal/windows_defines_errors.h"

#include "c_pal/error_handling.h"

static atomic_uint64_t last_error_code;

void error_handling_set_last_error(atomic_uint64_t err_code)
{

}

atomic_uint64_t error_handling_get_last_error(void)
{

}
