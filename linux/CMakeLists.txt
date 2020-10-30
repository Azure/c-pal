#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_common_h_files
    ../common/inc/c_pal/call_once.h
    ../common/inc/c_pal/lazy_init.h
)

set(pal_common_c_files
    ../common/src/call_once.c
    ../common/src/lazy_init.c
)

#determining which one of the GBALLOC_LL implementations to use. By convention the file is called "gballoc_ll_" followed by "type".
string(TOLOWER "${GBALLOC_LL_TYPE}" gballoc_ll_type_lower)
set(gballoc_ll_c gballoc_ll_${gballoc_ll_type_lower}.c)

#determining which one of the GBALLOC_HL implementations to use. By convention the file is called "gballoc_hl_" followed by "type".
string(TOLOWER "${GBALLOC_HL_TYPE}" gballoc_hl_type_lower)
set(gballoc_hl_c gballoc_hl_${gballoc_hl_type_lower}.c)

set(pal_linux_h_files
    ${pal_common_h_files}
)

set(pal_linux_c_files
    ${pal_common_c_files}
    src/interlocked_linux.c
    src/platform_linux.c
    src/threadapi_pthreads.c
    src/uniqueid_linux.c
    src/sync_linux.c
    src/string_utils.c
    src/file_linux.c
    src/timer_linux.c
    src/${gballoc_ll_c}
    src/${gballoc_hl_c}
)

FILE(GLOB pal_linux_md_files "devdoc/*.md")
FILE(GLOB pal_common_md_files "../common/devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_linux_md_files} ${pal_common_md_files})

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(../common/inc)

add_library(pal_linux ${pal_linux_h_files} ${pal_linux_c_files} ${pal_linux_md_files} ${pal_common_md_files})
target_link_libraries(pal_linux pal_interfaces rt uuid pthread)
target_include_directories(pal_linux PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)

add_subdirectory(linux_reals)
add_subdirectory(tests)
add_subdirectory(../common/tests common/tests)
