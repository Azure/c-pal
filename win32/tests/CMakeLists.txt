#Copyright (C) Microsoft Corporation. All rights reserved.

# unit tests
if(${run_unittests})
    add_subdirectory(execution_engine_win32_ut)
    add_subdirectory(async_socket_win32_ut)
    add_subdirectory(threadpool_win32_ut)
    add_subdirectory(interlocked_win32_ut)
    add_subdirectory(lock_win32_ut)
    add_subdirectory(interlocked_hl_win32_ut)
    add_subdirectory(platform_win32_ut)
    add_subdirectory(uniqueid_win32_ut)
    add_subdirectory(timer_win32_ut)
    add_subdirectory(srw_lock_win32_ut)
    add_subdirectory(reals_win32_ut)
    add_subdirectory(sync_win32_ut)
    add_subdirectory(file_win32_ut)
    
    build_test_folder(gballoc_hl_metrics_ut)
    build_test_folder(gballoc_hl_metrics_wout_init_ut)  
    
    build_test_folder(gballoc_ll_passthrough_ut)
    build_test_folder(gballoc_ll_win32heap_ut)
    
    build_test_folder(gballoc_ll_mimalloc_ut)
        
    build_test_folder(gballoc_hl_passthrough_ut)
    
    
endif()

if(${run_int_tests})
    add_subdirectory(async_socket_win32_int)
    add_subdirectory(threadpool_win32_int)
    add_subdirectory(string_utils_int)
        
    build_test_folder(gballoc_ll_mimalloc_int)
    build_test_folder(gballoc_ll_win32heap_int) 
    build_test_folder(gballoc_ll_passthrough_int)
    
    #this builds and runs the same test for gballoc_hl but with all the implementations of the underlying gballoc_ll.
    #this scripts sets 2 variables (GBALOC_LL_IMPL/GBALLOC_HL_IMPL) that is picked as underlying implementation for gballoc_hl/ll and consequentially as file name (gballoc_ll_${GBALOC_LL_IMPL}) (gballoc_hl_${GBALOC_HL_IMPL})
    foreach(GBALOC_HL_IMPL ${GBALLOC_HL_TYPE_VALUES})
        STRING(TOLOWER ${GBALOC_HL_IMPL} gballoc_hl_impl_lower)
        foreach(GBALOC_LL_IMPL ${GBALLOC_LL_TYPE_VALUES})
            
            STRING(TOLOWER ${GBALOC_LL_IMPL} gballoc_ll_impl_lower)
            build_test_folder(gballoc_hl_int BINARY_DIR ${gballoc_hl_impl_lower}_${gballoc_ll_impl_lower})
            
        endforeach()
    endforeach()
    
endif()

