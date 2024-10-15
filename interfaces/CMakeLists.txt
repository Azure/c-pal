#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_interfaces_h_files
    c_pal/arithmetic.h
    c_pal/error_handling.h
    c_pal/execution_engine.h
    c_pal/async_socket.h
    c_pal/file_util.h
    c_pal/socket_handle.h
    c_pal/threadpool.h
    c_pal/pipe.h
    c_pal/platform.h
    c_pal/timer.h
    c_pal/uuid.h
    c_pal/srw_lock.h
    c_pal/srw_lock_ll.h
    c_pal/file.h
    c_pal/gballoc_ll.h
    #c_pal/gballoc_ll_redirect.h #it can only be one of gballoc_ll_redirect.h or gballoc_hl_redirect.h and the HL version is actually used.
    c_pal/gballoc_hl.h
    c_pal/gballoc_hl_redirect.h
    c_pal/malloc_multi_flex.h
)

FILE(GLOB pal_interfaces_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_interfaces_md_files})

include_directories(../linux/inc) #for "pal_windows_defines.h" when compiling linux code.
include_directories(../common/inc) #for thandle.h which is used in threadpool.h

set(pal_interfaces_h_files_with_inc_prefix)
    foreach(file ${pal_interfaces_h_files})
        list(APPEND pal_interfaces_h_files_with_inc_prefix "inc/${file}")
    endforeach()

add_library(pal_interfaces INTERFACE ${pal_interfaces_h_files_with_inc_prefix})
target_include_directories(pal_interfaces INTERFACE ${CMAKE_CURRENT_LIST_DIR}/inc)
target_link_libraries(pal_interfaces INTERFACE pal_ll_interfaces macro_utils_c)

add_mock_library(pal_interfaces HEADER_FILES ${pal_interfaces_h_files})

add_subdirectory(reals)

add_subdirectory(tests)
