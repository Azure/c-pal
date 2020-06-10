#Copyright (C) Microsoft Corporation. All rights reserved.

# unit tests
if(${run_unittests})
    add_subdirectory(interlocked_linux_ut)
    add_subdirectory(uniqueid_ut)
endif()

if(${run_int_tests})
endif()
