// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*NOTE: real_execution_engine_linux_get_parameters is implemented as "real" in execution_engine.c*/
/*NOTE: so this file is empty awaiting for a normalization of that exception*/

#include "real_execution_engine_linux_renames.h" // IWYU pragma: keep

static int do_nothing = 0; /*to avoid warning C4206: nonstandard extension used: translation unit is empty*/

// Function is to avoid Warning -Werror=unused-variable in gcc
static void unused_var_function(void)
{
    do_nothing = 0;
}
