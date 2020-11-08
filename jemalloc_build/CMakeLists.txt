#Copyright (C) Microsoft Corporation. All rights reserved.

set(jemalloc_h_files
)

set(jemalloc_c_files
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/arena.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/background_thread.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/base.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/bin.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/bitmap.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/ckh.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/ctl.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/div.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/extent.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/extent_dss.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/extent_mmap.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/hash.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/hook.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/jemalloc.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/large.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/log.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/malloc_io.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/mutex.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/mutex_pool.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/nstime.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/pages.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/prng.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/prof.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/rtree.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/sc.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/stats.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/sz.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/tcache.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/test_hooks.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/ticker.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/tsd.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/witness.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/safety_check.c
)

include_directories(../deps/jemalloc/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/msvc_compat)

add_library(jemalloc ${jemalloc_h_files} ${jemalloc_c_files})

target_include_directories(jemalloc PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/include ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/include/msvc_compat)
