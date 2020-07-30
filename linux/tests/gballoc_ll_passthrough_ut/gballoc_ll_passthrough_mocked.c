// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define malloc mock_malloc
#define free mock_free
#define realloc mock_realloc
#define calloc mock_calloc
#define malloc_usable_size mock_malloc_usable_size

#include "../../src/gballoc_ll_passthrough.c"
