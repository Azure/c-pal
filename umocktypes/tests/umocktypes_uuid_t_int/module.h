// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef MODULE_H
#define MODULE_H

#include "c_pal/uuid.h"

#include "umock_c/umock_c_prod.h"

    /*these are just passthrough functions to submodule*/
    /* MOCKABLE_FUNCTION(, UUID_T, module_returns_UUID_T); => note functions cannot return array */

    MOCKABLE_FUNCTION(, void, module_writes_UUID_T, UUID_T, out);
    MOCKABLE_FUNCTION(, void, module_reads_UUID_T, UUID_T, u);
    MOCKABLE_FUNCTION(, void, module_reads_const_UUID_T, const UUID_T, u);

#endif // MODULE_H

