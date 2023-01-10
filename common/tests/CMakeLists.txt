#Copyright (C) Microsoft Corporation. All rights reserved.

if(MSVC)
    set(PAL_OS pal_win32)
else()
    set(PAL_OS pal_linux)
endif()

# unit tests
if(${run_unittests})
    build_test_folder(refcount_ut)
    build_test_folder(call_once_ut)
    build_test_folder(lazy_init_ut)
    build_test_folder(s_list_ut)
endif()

if(${run_int_tests})
    build_test_folder(arithmetic_int)
    build_test_folder(call_once_int)
    build_test_folder(lazy_init_int)
endif()


