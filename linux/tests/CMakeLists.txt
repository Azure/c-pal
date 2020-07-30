#Copyright (C) Microsoft Corporation. All rights reserved.

# unit tests
if(${run_unittests})
    build_test_folder(interlocked_linux_ut)
    build_test_folder(uniqueid_ut)
    build_test_folder(linux_reals_ut)
    build_test_folder(sync_linux_ut)
endif()

if(${run_int_tests})
endif()
