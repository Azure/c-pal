// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUE_THANDLE_H
#define UNIQUE_THANDLE_H

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle_ll.h"

/*macro to be used in headers*/
/*introduces UNIQUE_THANDLE(T) for a previously defined type T and declares the functions that move and dispose it*/
#define UNIQUE_THANDLE_TYPE_DECLARE(T) \
    UNIQUE_THANDLE_LL_TYPE_DECLARE(T, T)

#endif /*UNIQUE_THANDLE_H*/
