#Copyright (C) Microsoft Corporation. All rights reserved.

add_subdirectory(interfaces)

if(WIN32)
    add_subdirectory(win32)
else()
    add_subdirectory(linux)
endif()

add_library(c_pal_ll INTERFACE)

if(MSVC)
    target_link_libraries(c_pal_ll INTERFACE pal_ll_win32)
else()
    target_link_libraries(c_pal_ll INTERFACE pal_ll_linux)
endif()

add_library(c_pal_ll_reals INTERFACE)
if(MSVC)
    target_link_libraries(c_pal_ll_reals INTERFACE win32_ll_reals)
else()
    target_link_libraries(c_pal_ll_reals INTERFACE linux_ll_reals)
endif()

if(${run_reals_check})
    add_reals_check_target()
endif()

