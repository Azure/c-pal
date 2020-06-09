#Copyright (C) Microsoft Corporation. All rights reserved.

if(${run_ut_tests})
    add_subdirectory(lock_ut)
endif()

if(${run_int_tests})
    add_subdirectory(interlocked_int)
endif()
