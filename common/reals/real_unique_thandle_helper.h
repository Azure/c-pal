// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_UNIQUE_THANDLE_HELPER_H
#define REAL_UNIQUE_THANDLE_HELPER_H

#include "macro_utils/macro_utils.h"

#include "c_pal/unique_thandle.h"
#include "c_pal/unique_thandle_ll.h"

#include "c_pal/gballoc_hl.h"
#include "real_gballoc_hl.h"

// Declares REAL_TYPE, an alias of TYPE, and the UNIQUE_THANDLE functions of REAL_TYPE. They use UNIQUE_THANDLE(TYPE), introduced before by UNIQUE_THANDLE_TYPE_DECLARE(TYPE)
#define REAL_UNIQUE_THANDLE_DECLARE(TYPE) \
    typedef TYPE MU_C2(REAL_, TYPE); \
    UNIQUE_THANDLE_LL_TYPE_DECLARE(MU_C2(REAL_, TYPE), TYPE);

// Defines the UNIQUE_THANDLE functions of REAL_TYPE
// Must have ENABLE_MOCKS undefined
// Recommend including real_interlocked_renames.h before this call and real_interlocked_undo_rename.h after this call
#define REAL_UNIQUE_THANDLE_DEFINE(TYPE) \
    UNIQUE_THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(MU_C2(REAL_, TYPE), TYPE, real_gballoc_hl_malloc, real_gballoc_hl_malloc_flex, real_gballoc_hl_free);

// Hooks the mocked UNIQUE_THANDLE functions of TYPE to the UNIQUE_THANDLE functions of REAL_TYPE
#define REGISTER_REAL_UNIQUE_THANDLE_MOCK_HOOK(TYPE) \
    REGISTER_GLOBAL_MOCK_HOOK(UNIQUE_THANDLE_INITIALIZE_MOVE(TYPE), UNIQUE_THANDLE_INITIALIZE_MOVE(MU_C2(REAL_, TYPE))); \
    REGISTER_GLOBAL_MOCK_HOOK(UNIQUE_THANDLE_DISPOSE(TYPE), UNIQUE_THANDLE_DISPOSE(MU_C2(REAL_, TYPE)));

// Registers UNIQUE_THANDLE(TYPE) and const UNIQUE_THANDLE(TYPE) with umock_c as aliases of void*, so that a mocked function stores the address of the handle it receives
#define REGISTER_UNIQUE_THANDLE_UMOCK_ALIAS_TYPES(TYPE) \
    REGISTER_UMOCK_ALIAS_TYPE(UNIQUE_THANDLE(TYPE), void*); \
    REGISTER_UMOCK_ALIAS_TYPE(const UNIQUE_THANDLE(TYPE), void*);

#endif //REAL_UNIQUE_THANDLE_HELPER_H
