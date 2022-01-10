// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "submodule.h"

#include "module.h"

void module_writes_UUID_T(UUID_T out)
{

    /*actually delegates it to submodule*/
    submodule_writes_UUID_T(out);
}

void module_reads_UUID_T(const UUID_T u)
{
    (void)u;

    UUID_T u_base = {
        (unsigned char)'a',
        (unsigned char)'b',
        (unsigned char)'c',
        (unsigned char)'d',
        (unsigned char)'e',
        (unsigned char)'f',
        (unsigned char)'g',
        (unsigned char)'h',
        (unsigned char)'i',
        (unsigned char)'j',
        (unsigned char)'k',
        (unsigned char)'l',
        (unsigned char)'m',
        (unsigned char)'n',
        (unsigned char)'o',
        (unsigned char)'p'
    };

    submodule_reads_UUID_T(u_base);
}

