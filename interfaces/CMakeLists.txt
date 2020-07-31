#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_interfaces_h_files
    inc/azure_c_pal/execution_engine.h
    inc/azure_c_pal/async_socket.h
    inc/azure_c_pal/socket_handle.h
    inc/azure_c_pal/threadpool.h
    inc/azure_c_pal/interlocked.h
    inc/azure_c_pal/interlocked_hl.h
    inc/azure_c_pal/platform.h
    inc/azure_c_pal/threadapi.h
    inc/azure_c_pal/timer.h
    inc/azure_c_pal/uniqueid.h
    inc/azure_c_pal/srw_lock.h
    inc/azure_c_pal/sync.h
    inc/azure_c_pal/file.h
    inc/azure_c_pal/gballoc_ll.h
    inc/azure_c_pal/gballoc_ll_redirects.h
    inc/azure_c_pal/gballoc_hl.h
    inc/azure_c_pal/gballoc_hl_redirects.h
)

FILE(GLOB pal_interfaces_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_interfaces_md_files})

set(PAL_INTERFACES_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using the PAL interfaces" FORCE)

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(${MACRO_UTILS_INC_FOLDER})

add_library(pal_interfaces INTERFACE)
target_include_directories(pal_interfaces INTERFACE ${CMAKE_CURRENT_LIST_DIR}/inc)

add_subdirectory(reals)

add_subdirectory(tests)
