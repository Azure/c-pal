#Copyright (C) Microsoft Corporation. All rights reserved.

set(jemalloc_h_files
)

set(jemalloc_c_files
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/arena.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/background_thread.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/base.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/bin.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/bin_info.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/bitmap.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/buf_writer.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/cache_bin.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/ckh.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/counter.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/ctl.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/decay.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/div.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/ecache.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/edata.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/edata_cache.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/ehooks.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/emap.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/eset.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/extent.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/extent_dss.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/extent_mmap.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/hook.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/inspect.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/jemalloc.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/large.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/log.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/malloc_io.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/mutex.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/mutex_pool.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/nstime.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/pa.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/pa_extra.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/pac.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/pages.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/peak_event.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/prof.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/prof_data.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/prof_log.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/prof_recent.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/prof_sys.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/rtree.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/safety_check.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/sc.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/stats.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/sz.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/tcache.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/test_hooks.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/thread_event.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/tsd.c
    ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/src/witness.c
)

include_directories(../deps/jemalloc/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/msvc_compat)

add_library(jemalloc ${jemalloc_h_files} ${jemalloc_c_files})

target_include_directories(jemalloc PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../deps/jemalloc/include ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/include/msvc_compat)
