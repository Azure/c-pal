#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_interfaces_h_files
    inc/execution_engine.h
    inc/async_socket.h
    inc/socket_handle.h
    inc/threadpool.h
    inc/interlocked.h
    inc/interlocked_hl.h
    inc/lock.h
    inc/platform.h
    inc/threadapi.h
    inc/timer.h
    inc/uniqueid.h
    inc/srw_lock.h
    inc/string_utils.h
    inc/sync.h
)

FILE(GLOB pal_interfaces_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_interfaces_md_files})

set(PAL_INTERFACES_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using the PAL interfaces" FORCE)

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(${MACRO_UTILS_INC_FOLDER})

add_library(pal_interfaces INTERFACE)
target_include_directories(pal_interfaces INTERFACE ${CMAKE_CURRENT_LIST_DIR}/inc)
add_subdirectory(tests)
