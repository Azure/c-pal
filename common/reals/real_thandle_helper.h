// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_THANDLE_HELPER_H
#define REAL_THANDLE_HELPER_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"
#include "c_pal/thandle_ll.h"

#include "c_pal/gballoc_hl.h"
#include "real_gballoc_hl.h"

// Creates a THANDLE alias of TYPE called REAL_TYPE
// Must have ENABLE_MOCKS undefined
// Recommend including real_interlocked_renames.h before this call and real_interlocked_undo_renames.h after this call
#define REAL_THANDLE_DEFINE(TYPE) \
    THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(MU_C2(REAL_, TYPE), TYPE, real_gballoc_hl_malloc, real_gballoc_hl_malloc_flex, real_gballoc_hl_free);

#define REAL_THANDLE_DECLARE(TYPE) \
    typedef TYPE MU_C2(REAL_, TYPE); \
    THANDLE_LL_TYPE_DECLARE(MU_C2(REAL_, TYPE), TYPE); \

// Hook the THANDLE(TYPE) calls to the THANDLE(REAL_TYPE) calls
#define REGISTER_REAL_THANDLE_MOCK_HOOK(TYPE) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(TYPE), THANDLE_ASSIGN(MU_C2(REAL_, TYPE))); \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(TYPE), THANDLE_INITIALIZE(MU_C2(REAL_, TYPE))); \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(TYPE), THANDLE_INITIALIZE_MOVE(MU_C2(REAL_, TYPE))); \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(TYPE), THANDLE_MOVE(MU_C2(REAL_, TYPE)));

#endif //REAL_THANDLE_HELPER_H
