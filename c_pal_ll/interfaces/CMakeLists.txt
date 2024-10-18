#Copyright (C) Microsoft Corporation. All rights reserved.

set(pal_ll_interfaces_h_files
    c_pal/interlocked.h
    c_pal/interlocked_macros.h
    c_pal/sync.h
    c_pal/threadapi.h
)

FILE(GLOB pal_ll_interfaces_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${pal_ll_interfaces_md_files})

set(pal_ll_interfaces_h_files_with_inc_prefix)
    foreach(file ${pal_ll_interfaces_h_files})
        list(APPEND pal_ll_interfaces_h_files_with_inc_prefix "inc/${file}")
    endforeach()

add_library(pal_ll_interfaces INTERFACE
    ${pal_ll_interfaces_h_files_with_inc_prefix}
)
target_include_directories(pal_ll_interfaces INTERFACE inc)

add_subdirectory(reals)

add_subdirectory(tests)
