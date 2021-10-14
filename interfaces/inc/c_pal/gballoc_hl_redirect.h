// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef GBALLOC_HL_REDIRECT_H
#define GBALLOC_HL_REDIRECT_H

#ifdef __cplusplus
#include <cstdlib> // IWYU pragma: keep
#else
#include <stdlib.h> // IWYU pragma: keep
#endif

/*this file provides a convenient way of having all the malloc/free preprocessor tokens in a source file replaced by their gballoc_hl counterparts*/

#ifdef GBALLOC_LL_REDIRECT_H
#error only one of gballoc_ll_redirect.h / gballoc_hl_redirect.h can be included at a time. gballoc_ll_redirect.h was included before gballoc_hl_redirect.h (this file)
#endif

#define malloc gballoc_hl_malloc
#define malloc_2 gballoc_hl_malloc_2
#define malloc_flex gballoc_hl_malloc_flex
#define free gballoc_hl_free
#define calloc gballoc_hl_calloc
#define realloc gballoc_hl_realloc
#define realloc_2 gballoc_hl_realloc_2
#define realloc_flex gballoc_hl_realloc_flex

#endif /* GBALLOC_HL_REDIRECT_H */
