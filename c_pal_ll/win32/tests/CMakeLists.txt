#Copyright (C) Microsoft Corporation. All rights reserved.

# unit tests
if(${run_unittests})
    build_test_folder(interlocked_win32_ut)
    build_test_folder(sync_win32_ut)
endif()

if(${run_int_tests})
endif()

