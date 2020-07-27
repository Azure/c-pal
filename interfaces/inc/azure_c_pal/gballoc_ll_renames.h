// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef GBALLOC_LL_RENAMES_H
#define GBALLOC_LL_RENAMES_H

/*this file provides a convenient way of having all the malloc/free preprocessor tokens in a source file replaced by their gballoc_ll counterparts*/

#define malloc gballoc_ll_malloc
#define free gballoc_ll_free
#define calloc gballoc_ll_calloc
#define realloc gballoc_ll_realloc

#endif /* GBALLOC_LL_RENAMES_H */
