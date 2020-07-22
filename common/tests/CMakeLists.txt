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
    add_subdirectory(gballoc_hl_metrics_ut)
    add_subdirectory(gballoc_hl_metrics_wout_init_ut)
    
    if(
        (${PAL_OS} STREQUAL "pal_win32") AND
        (${ARCHITECTURE} STREQUAL "x86_64")
    )
        build_test_folder(gballoc_ll_passthrough_ut)
        build_test_folder(gballoc_ll_win32heap_ut)
        build_test_folder(gballoc_ll_mimalloc_ut)
        
        build_test_folder(gballoc_hl_passthrough_ut)
    endif()
endif()

if(${run_int_tests})
    if(
        (${PAL_OS} STREQUAL "pal_win32") AND
        (${ARCHITECTURE} STREQUAL "x86_64")
    )
        build_test_folder(gballoc_ll_mimalloc_int)
        build_test_folder(gballoc_ll_win32heap_int) 
        build_test_folder(gballoc_ll_passthrough_int)
        
        #this builds and runs the same test for gballoc_hl but with all the implementations of the underlying gballoc_ll.
        #this scripts sets a variable (GBALOC_LL_IMPL) that is picked as underlying implementatio of gballoc_ll and consequentially as file name (gballoc_ll_${GBALOC_LL_IMPL})
        foreach(GBALOC_LL_IMPL ${GBALLOC_LL_TYPE_VALUES})
            STRING(TOLOWER ${GBALOC_LL_IMPL} gballoc_ll_impl_lower)
            build_test_folder(gballoc_hl_passthrough_int BINARY_DIR ${gballoc_ll_impl_lower})
        endforeach()
        
    endif()
endif()

add_subdirectory(gballoc_hl_perf)