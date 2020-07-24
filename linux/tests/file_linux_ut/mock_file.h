// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <aio.h>

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, int, mock_open, const char*, pathname, int, flags, int, mode);
MOCKABLE_FUNCTION(, int, mock_close, int, fildes);
MOCKABLE_FUNCTION(, int, mock_aio_write, void*, aiocbp);
MOCKABLE_FUNCTION(, int, mock_aio_read, void*, aiocbp);
MOCKABLE_FUNCTION(, int, mock_aio_return, void*, aiocbp);
#ifdef __cplusplus
}
#endif

