// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif
    MOCKABLE_FUNCTION(, BOOL, mocked_FileTimeToSystemTime, const FILETIME*, lpFileTime, LPSYSTEMTIME, lpSystemTime);



#ifdef __cplusplus
}
#endif

