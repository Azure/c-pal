// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define mi_malloc mock_mi_malloc
#define mi_free mock_mi_free
#define mi_calloc mock_mi_calloc
#define mi_realloc mock_mi_realloc

#include "../../src/gballoc_ll_mimalloc.c"
