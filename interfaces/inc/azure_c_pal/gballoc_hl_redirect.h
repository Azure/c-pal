// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef GBALLOC_HL_REDIRECT_H
#define GBALLOC_HL_REDIRECT_H

/*this file provides a convenient way of having all the malloc/free preprocessor tokens in a source file replaced by their gballoc_hl counterparts*/

#ifdef GBALLOC_LL_REDIRECT_H
#error only one of gballoc_ll_redirect.h / gballoc_hl_redirect.h can be included at a time. gballoc_ll_redirect.h was included before gballoc_hl_redirect.h (this file)
#endif

#define malloc gballoc_ll_malloc
#define free gballoc_ll_free
#define calloc gballoc_ll_calloc
#define realloc gballoc_ll_realloc

#endif /* GBALLOC_HL_REDIRECT_H */
