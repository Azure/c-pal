// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define timer_create_new                real_timer_create_new
#define timer_start                     real_timer_start
#define timer_get_elapsed               real_timer_get_elapsed
#define timer_get_elapsed_ms            real_timer_get_elapsed_ms
#define timer_destroy                   real_timer_destroy
#define timer_global_get_elapsed_s      real_timer_global_get_elapsed_s
#define timer_global_get_elapsed_ms     real_timer_global_get_elapsed_ms
#define timer_global_get_elapsed_us     real_timer_global_get_elapsed_us
