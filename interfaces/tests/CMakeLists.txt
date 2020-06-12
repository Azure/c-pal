#Copyright (C) Microsoft Corporation. All rights reserved.

add_subdirectory(reals)

if(${run_ut_tests})
    add_subdirectory(lock_ut)
endif()

if(${run_int_tests})
    add_subdirectory(interlocked_int)
endif()
