#Copyright (C) Microsoft Corporation. All rights reserved.

if(${run_unittests})
    build_test_folder(interlocked_macros_ut)
endif()

if(${run_int_tests})
    build_test_folder(interlocked_int)
    build_test_folder(sync_int)
endif()


