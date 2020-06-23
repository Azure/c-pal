#Copyright (C) Microsoft Corporation. All rights reserved.

add_subdirectory(reals_linux)
# unit tests
if(${run_unittests})
    add_subdirectory(interlocked_linux_ut)
    add_subdirectory(uniqueid_ut)
    add_subdirectory(reals_linux_ut)
    add_subdirectory(sync_linux_ut)
endif()

if(${run_int_tests})
endif()
