#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_common_h_files
    ../common/inc/gballoc.h
)

set(pal_common_c_files
    ../common/src/gballoc.c
)

set(pal_linux_h_files
    ${pal_common_h_files}
    inc/refcount_os.h
)

set(pal_linux_c_files
    ${pal_common_c_files}
    src/interlocked_linux.c
    src/lock_pthreads.c
    src/platform_linux.c
    src/threadapi_pthreads.c
    src/uniqueid_linux.c
    src/sync_linux.c
    src/file_linux.c
)

FILE(GLOB pal_linux_md_files "devdoc/*.md")
FILE(GLOB pal_common_md_files "../common/devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_linux_md_files} ${pal_common_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(../common/inc)

add_library(pal_linux ${pal_linux_h_files} ${pal_linux_c_files} ${pal_linux_md_files} ${pal_common_md_files})
target_link_libraries(pal_linux pal_interfaces rt uuid)
target_include_directories(pal_linux PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)

add_subdirectory(tests)
add_subdirectory(../common/tests common/tests)
