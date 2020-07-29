#Copyright (C) Microsoft Corporation. All rights reserved.

add_subdirectory(linux_reals)
# unit tests
if(${run_unittests})
    build_test_folder(interlocked_linux_ut)
    build_test_folder(uniqueid_ut)
    build_test_folder(reals_linux_ut)
    build_test_folder(sync_linux_ut)
endif()

if(${run_int_tests})
endif()
