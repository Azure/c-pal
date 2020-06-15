#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

if (TARGET azure_c_pal)
    RETURN()
endif()

project(azure_c_pal)

#the following variables are project-wide and can be used with cmake-gui
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF). Chsare dutility does not have any e2e tests, but the option needs to exist to evaluate in IF statements" OFF)
option(run_int_tests "set run_int_tests to ON to integration tests (default is OFF)." OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is ON)" ON)
option(run_traceability "run traceability tool (default is ON)" ON)

set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_unittests ${run_unittests})
set(original_run_int_tests ${run_int_tests})
set(original_run_traceability ${run_traceability})
set(original_run_perf_tests ${run_perf_tests})


set(run_e2e_tests OFF)
set(run_unittests OFF)
set(run_int_tests OFF)
set(run_traceability OFF)
set(run_perf_tests OFF)


if ((NOT TARGET azure_c_build_tools) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-build-tools/CMakeLists.txt))
        add_subdirectory(deps/azure-c-build-tools)
endif()
set_default_build_options()

include(CTest)
enable_testing()

set(C_PAL_INC_FOLDER 
    ${CMAKE_CURRENT_LIST_DIR}/interfaces/inc
    ${CMAKE_CURRENT_LIST_DIR}/common/inc
    CACHE INTERNAL "this is what needs to be included if using azure_c_pal" FORCE)

if ((NOT TARGET azure_macro_utils_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-macro-utils-c/CMakeLists.txt))
        add_subdirectory(deps/azure-macro-utils-c)
        include_directories(deps/azure-macro-utils-c/inc)
endif()

if ((NOT TARGET azure_c_logging) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-logging/CMakeLists.txt))
        add_subdirectory(deps/azure-c-logging)
        include_directories(deps/azure-c-logging/inc)
endif()

if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-ctest/CMakeLists.txt))
        add_subdirectory(deps/azure-ctest)
        include_directories(deps/azure-ctest/inc)
endif()

if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-testrunnerswitcher/CMakeLists.txt))
        add_subdirectory(deps/azure-c-testrunnerswitcher)
        include_directories(deps/azure-c-testrunnerswitcher/inc)
endif()

if ((NOT TARGET umock_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/umock-c/CMakeLists.txt))
        add_subdirectory(deps/umock-c)
        include_directories(deps/umock-c/inc)
endif()

set(run_e2e_tests ${original_run_e2e_tests})
set(run_unittests ${original_run_unittests})
set(run_int_tests ${original_run_int_tests})
set(run_traceability ${original_run_traceability})
set(run_perf_tests ${original_run_perf_tests})

add_subdirectory(interfaces)
add_subdirectory(common)
if(WIN32)
    add_subdirectory(win32)
else()
    add_subdirectory(linux)
endif()


add_library(azure_c_pal INTERFACE)

FILE(GLOB azure_c_pal_md_files "doc/*.md")
SOURCE_GROUP(devdoc FILES ${azure_c_pal_md_files})
target_sources(azure_c_pal INTERFACE ${azure_c_pal_md_files})

if(MSVC)
    target_link_libraries(azure_c_pal INTERFACE pal_win32)
else()
    target_link_libraries(azure_c_pal INTERFACE pal_linux)
endif()


if((WIN32) AND (${run_traceability}))
    #add traceability custom target
    add_custom_target(azure_c_pal_traceability ALL
        COMMAND traceabilitytool -buildcheck -e ${CMAKE_CURRENT_LIST_DIR}/deps -i ${CMAKE_CURRENT_LIST_DIR})
    add_dependencies(azure_c_pal_traceability traceabilitytool)
endif()

add_library(azure_c_pal_reals INTERFACE)
if(MSVC)
    target_link_libraries(azure_c_pal_reals INTERFACE reals_win32)
else()
    target_link_libraries(azure_c_pal_reals INTERFACE reals_linux)
endif()

include(CMakePackageConfigHelpers)

