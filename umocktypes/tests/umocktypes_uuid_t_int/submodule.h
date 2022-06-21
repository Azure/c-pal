// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SUBMODULE_H
#define SUBMODULE_H

#include "c_pal/uuid.h"

#include "umock_c/umock_c_prod.h"

    MOCKABLE_FUNCTION(, void, submodule_writes_UUID_T, UUID_T, u);
    MOCKABLE_FUNCTION(, void, submodule_reads_UUID_T, UUID_T, u);
    MOCKABLE_FUNCTION(, void, submodule_reads_const_UUID_T, const UUID_T, u);

#endif // SUBMODULE_H

