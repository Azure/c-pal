// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SUBMODULE_H
#define SUBMODULE_H

#include "c_pal/uuid.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, void, submodule_writes_UUID_T, UUID_T, u);
    MOCKABLE_FUNCTION(, void, submodule_reads_UUID_T, const UUID_T, u);

#ifdef __cplusplus
}
#endif

#endif // SUBMODULE_H

