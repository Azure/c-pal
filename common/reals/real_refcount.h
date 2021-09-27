// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_REFCOUNT_H
#define REAL_REFCOUNT_H

#include "c_pal/refcount.h"

#include "real_interlocked.h"

#ifdef __cplusplus
extern "C" {
#endif

// When this file is included, test types defined as refcount types will not generate mock calls to interlocked

#undef INC_REF
#define INC_REF(type, var) real_interlocked_increment(&((REFCOUNT_TYPE(type)*)((unsigned char*)var - offsetof(REFCOUNT_TYPE(type), counted)))->count)
#undef DEC_REF
#define DEC_REF(type, var) real_interlocked_decrement(&((REFCOUNT_TYPE(type)*)((unsigned char*)var - offsetof(REFCOUNT_TYPE(type), counted)))->count)
#undef INIT_REF
#define INIT_REF(type, var) real_interlocked_exchange(&((REFCOUNT_TYPE(type)*)((unsigned char*)var - offsetof(REFCOUNT_TYPE(type), counted)))->count, 1)

#ifdef __cplusplus
}
#endif

#endif //REAL_REFCOUNT_H
