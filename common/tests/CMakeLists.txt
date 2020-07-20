#Copyright (C) Microsoft Corporation. All rights reserved.

if(MSVC)
    set(PAL_OS pal_win32)
else()
    set(PAL_OS pal_linux)
endif()

# unit tests
if(${run_unittests})
    add_subdirectory(gballoc_ut)
    add_subdirectory(gballoc_without_init_ut)
    add_subdirectory(refcount_ut)
    build_test_folder(gballoc_ll_passthrough_ut)
    if(${PAL_OS} STREQUAL "pal_win32")
        build_test_folder(gballoc_ll_win32heap_ut)
    endif()
endif()

if(${run_int_tests})
endif()
