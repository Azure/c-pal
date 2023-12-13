// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define gballoc_hl_init                          real_gballoc_hl_init
#define gballoc_hl_deinit                        real_gballoc_hl_deinit
#define gballoc_hl_malloc                        real_gballoc_hl_malloc
#define gballoc_hl_malloc_2                      real_gballoc_hl_malloc_2
#define gballoc_hl_malloc_flex                   real_gballoc_hl_malloc_flex
#define gballoc_hl_calloc                        real_gballoc_hl_calloc
#define gballoc_hl_realloc                       real_gballoc_hl_realloc
#define gballoc_hl_realloc_2                     real_gballoc_hl_realloc_2
#define gballoc_hl_realloc_flex                  real_gballoc_hl_realloc_flex
#define gballoc_hl_free                          real_gballoc_hl_free
#define gballoc_hl_size                          real_gballoc_hl_size
#define gballoc_hl_reset_counters                real_gballoc_hl_reset_counters
#define gballoc_hl_get_malloc_latency_buckets    real_gballoc_hl_get_malloc_latency_buckets
#define gballoc_hl_get_realloc_latency_buckets   real_gballoc_hl_get_realloc_latency_buckets
#define gballoc_hl_get_calloc_latency_buckets    real_gballoc_hl_get_calloc_latency_buckets
#define gballoc_hl_get_free_latency_buckets      real_gballoc_hl_get_free_latency_buckets
#define gballoc_hl_get_latency_bucket_metadata   real_gballoc_hl_get_latency_bucket_metadata
#define gballoc_hl_print_stats                   real_gballoc_hl_print_stats
