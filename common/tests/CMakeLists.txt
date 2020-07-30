#Copyright (C) Microsoft Corporation. All rights reserved.

if(MSVC)
    set(PAL_OS pal_win32)
else()
    set(PAL_OS pal_linux)
endif()

# unit tests
if(${run_unittests})
    build_test_folder(refcount_ut)
endif()

if(${run_int_tests})
    
endif()


