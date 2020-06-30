#Copyright (C) Microsoft Corporation. All rights reserved.

if(MSVC)
    set(PAL_OS pal_win32)
else()
    set(PAL_OS pal_linux)
endif()

# unit tests
if(${run_unittests})
    add_subdirectory(gballoc_ut)
    add_subdirectory(gballoc_without_init_ut)
    add_subdirectory(refcount_ut)
    add_subdirectory(constbuffer_ut)
    add_subdirectory(buffer_ut)
endif()

if(${run_int_tests})
endif()
