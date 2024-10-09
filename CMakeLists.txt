#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT DEFINED CMAKE_MINIMUM_REQUIRED_VERSION)
    cmake_minimum_required(VERSION 3.18)
endif()

# canon way of using c-pal from another repo is below. It assumes the using repo has placed c-pal in "deps"
# here's an example command line: D:\r\COMPG\deps>git submodule add https://github.com/Azure/c-pal c-pal

#if ((NOT TARGET c_pal) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-pal/CMakeLists.txt))
#    add_subdirectory(deps/c-pal)
#    include_directories(${C_PAL_INC_FOLDER})
#endif()

if (TARGET c_pal)
    RETURN()
endif()

project(c_pal)

#the following variables are project-wide and can be used with cmake-gui
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF). Chsare dutility does not have any e2e tests, but the option needs to exist to evaluate in IF statements" OFF)
option(run_int_tests "set run_int_tests to ON to integration tests (default is OFF)." OFF)
option(run_perf_tests "set run_perf_tests to ON to run perf tests (default is OFF)." OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is OFF)" OFF)
option(run_traceability "run traceability tool (default is ON)" ON)
option(run_reals_check "set run_reals_check to ON to run reals check (default is OFF)." OFF)

if((NOT "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo") AND (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug"))
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be specified and be precisely one of Debug or RelWithDebInfo")
endif()

# This is done because jemalloc is obtained from vcpkg and brought in with PkgConfig
# PkgConfig does not support defining different variables with the libraries to be linked for different configurations
# This forces us to only be able to generate one build configuration (either Debug or RelWithDebInfo)
# Setting CMAKE_CONFIGURATION_TYPES is the way to limit what configurations are generated
# so that the user gets only one configuration as option in the Visual Studio GUI
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "" FORCE)

if (NOT DEFINED ENV{BUILD_BINARIESDIRECTORY})
    MESSAGE(FATAL_ERROR "cannot find BUILD_BINARIESDIRECTORY env variable which is absolutely needed. It can point to any folder where the binaries will pe placed. Here's an example: \nWindows:\nset BUILD_BINARIESDIRECTORY=d:\\BUILD_BINARIESDIRECTORY\nWSL:\nexport BUILD_BINARIESDIRECTORY=/mnt/d/r/BUILD_BINARIESDIRECTORY/linux")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG $ENV{BUILD_BINARIESDIRECTORY}/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO $ENV{BUILD_BINARIESDIRECTORY}/RelWithDebInfo)

# enabling this option can help with debugging latency issues for the networking
option(enable_socket_logging "enable logging of each packet send/receive (default is OFF)" OFF)

if(${enable_socket_logging})
    add_definitions(-DENABLE_SOCKET_LOGGING)
endif()

#canon way of limiting an option to a predefined set of values
set(GBALLOC_LL_TYPE_VALUES PASSTHROUGH WIN32HEAP MIMALLOC JEMALLOC) #the list of values which are allowed
set(GBALLOC_LL_TYPE PASSTHROUGH CACHE STRING "Value of GBALLOC_LL_TYPE") #setting the property's default value in case none is taken from the command line
set_property(CACHE GBALLOC_LL_TYPE PROPERTY STRINGS ${GBALLOC_LL_TYPE_VALUES}) #build a property that can have only the allowed values


list(FIND GBALLOC_LL_TYPE_VALUES ${GBALLOC_LL_TYPE} index) #check that the received value (either the default or the one from command line) matches one of the allowed values.
if(index EQUAL -1)
    message(FATAL_ERROR "GBALLOC_LL_TYPE must be one of '${GBALLOC_LL_TYPE_VALUES}'. It was actually '${GBALLOC_LL_TYPE}'")
endif()

#canon way of limiting an option to a predefined set of values
set(GBALLOC_HL_TYPE_VALUES PASSTHROUGH METRICS) #the list of values which are allowed
set(GBALLOC_HL_TYPE PASSTHROUGH CACHE STRING "Value of GBALLOC_HL_TYPE") #build a property that can have only the allowed values
set_property(CACHE GBALLOC_HL_TYPE PROPERTY STRINGS ${GBALLOC_HL_TYPE_VALUES}) #setting the property's default value in case none is taken from the command line

list(FIND GBALLOC_HL_TYPE_VALUES ${GBALLOC_HL_TYPE} index) #check that the received value (either the default or the one from command line) matches one of the allowed values.
if(index EQUAL -1)
    message(FATAL_ERROR "GBALLOC_HL_TYPE must be one of '${GBALLOC_HL_TYPE_VALUES}'. It was actually '${GBALLOC_HL_TYPE}'")
endif()

#bring in dependencies
#do not add or build any tests of the dependencies
set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_unittests ${run_unittests})
set(original_run_int_tests ${run_int_tests})
set(original_run_traceability ${run_traceability})
set(original_run_perf_tests ${run_perf_tests})
set(original_run_reals_check ${run_reals_check})

set(run_e2e_tests OFF)
set(run_unittests OFF)
set(run_int_tests OFF)
set(run_traceability OFF)
set(run_perf_tests OFF)
set(run_reals_check OFF)

if ((NOT TARGET c_build_tools) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-build-tools/CMakeLists.txt))
    add_subdirectory(deps/c-build-tools)
    set_default_build_options()
endif()

if ((WIN32) AND ("${GBALLOC_LL_TYPE}" STREQUAL "JEMALLOC"))
    # Bring in vcpkg
    use_vcpkg(${CMAKE_CURRENT_LIST_DIR}/deps/vcpkg)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules (JEMALLOC jemalloc)

    pkg_search_module(JEMALLOC REQUIRED jemalloc)
    include_directories(${JEMALLOC_INCLUDE_DIRS})

    #this wraps the jemalloc library in a target so it can be used in the rest of the build easily
    add_library(jemalloc INTERFACE)

    # The pkg_search_module above sets the variables needed for the include and link directories
    # we are interested to have the include directories produced by the pkg_search_module for jemalloc used so that
    # we can find the jemalloc headers
    # Also we need to link in our wrapper target the jemalloc static library, which lives at the location produced by
    # the pkg_search_module in ${pkgcfg_lib_JEMALLOC_jemalloc_s}
    # Note that PkgConfig is not geared to produce different variables pointing to different libs for different configurations
    # so the same variable will be used both debug and release (this forces us to only generate the CMakes for one build configuration only).
    target_link_libraries(jemalloc INTERFACE ${pkgcfg_lib_JEMALLOC_jemalloc_s})
    target_include_directories(jemalloc INTERFACE ${JEMALLOC_INCLUDE_DIRS})

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DJEMALLOC_NO_PRIVATE_NAMESPACE /D_REENTRANT /DJEMALLOC_EXPORT= /D_LIB")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DJEMALLOC_NO_PRIVATE_NAMESPACE /D_REENTRANT /DJEMALLOC_EXPORT= /D_LIB")
endif()

if ((NOT TARGET macro_utils_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/macro-utils-c/CMakeLists.txt))
    add_subdirectory(deps/macro-utils-c)
    include_directories(${MACRO_UTILS_INC_FOLDER})
endif()

if ((NOT TARGET c_logging_v2) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-logging/CMakeLists.txt))
    add_subdirectory(deps/c-logging)
    include_directories(deps/c-logging/v2/inc)
endif()

if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/ctest/CMakeLists.txt))
    add_subdirectory(deps/ctest)
    include_directories(${CTEST_INC_FOLDER})
endif()

if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-testrunnerswitcher/CMakeLists.txt))
    add_subdirectory(deps/c-testrunnerswitcher)
    include_directories(${TESTRUNNERSWITCHER_INC_FOLDER})
endif()

if ((NOT TARGET umock_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/umock-c/CMakeLists.txt))
    add_subdirectory(deps/umock-c)
    include_directories(${UMOCK_C_INC_FOLDER})
endif()

# Skip for Linux iwyu
if (WIN32)
    if (
        (NOT TARGET mimalloc-obj) AND
        (${GBALLOC_LL_TYPE} STREQUAL "MIMALLOC") AND
        (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/mimalloc/CMakeLists.txt)
        )
            set(MI_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE) #not building a dll allows building on 32 bit, otherwise there's some errors on init.c about not finding a imported symbol
            set(MI_BUILD_TESTS OFF CACHE BOOL "Build test executables" FORCE)
            #for mimalloc disable this warning: Warning C4459: declaration of 'os_page_size' hides global declaration
            #for mimalloc disable this warning: Warning C4100: 'try_alignment': unreferenced formal parameter
            #for mimalloc disable this warning: warning C4505: 'mi_os_get_aligned_hint': unreferenced local function has been removed

            set(PREV_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
            set(PREV_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
            if(WIN32)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4459 /wd4100 /wd4505")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4459 /wd4100 /wd4505")
            endif()

            add_subdirectory(deps/mimalloc)
            include_directories(deps/mimalloc/include)

            set(CMAKE_C_FLAGS ${PREV_CMAKE_C_FLAGS})
            set(CMAKE_CXX_FLAGS ${PREV_CMAKE_CXX_FLAGS})
    endif()
endif()

set(run_e2e_tests ${original_run_e2e_tests})
set(run_unittests ${original_run_unittests})
set(run_int_tests ${original_run_int_tests})
set(run_traceability ${original_run_traceability})
set(run_perf_tests ${original_run_perf_tests})
set(run_reals_check ${original_run_reals_check})

include(CTest)
enable_testing()

set(C_PAL_INC_FOLDER
    ${CMAKE_CURRENT_LIST_DIR}/interfaces/inc
    ${CMAKE_CURRENT_LIST_DIR}/common/inc
    ${CMAKE_CURRENT_LIST_DIR}/c_pal_ll/interfaces/inc
    CACHE INTERNAL "this is what needs to be included if using c_pal" FORCE)

#this function takes as arguments a LIBRARY_NAME followed by a list of HEADER_FILES
#it builds a separate header file which includes all the HEADER_FILES under the ENABLE_MOCKS_DECL #define
#it builds a separate source file which is build under the ENABLE_MOCKS #define
#it adds the header file and the source file to a library called ${LIBRARY_NAME}_mocks
#it adds the headers as precompiles headers for the ${LIBRARY_NAME}_mocks library
function(add_mock_library LIBRARY_NAME)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs HEADER_FILES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(MOCKS_HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME}_mocks.h)
    set(MOCKS_SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME}_mocks.c)

    file(WRITE ${MOCKS_HEADER_FILE} "#ifndef ${LIBRARY_NAME}_H\n")
    file(APPEND ${MOCKS_HEADER_FILE} "#define ${LIBRARY_NAME}_H\n\n")

    file(APPEND ${MOCKS_HEADER_FILE} "#define ENABLE_MOCKS\n")
    file(APPEND ${MOCKS_HEADER_FILE} "#define ENABLE_MOCKS_DECL\n")

    foreach(HEADER_FILE ${ARG_HEADER_FILES})
        file(APPEND ${MOCKS_HEADER_FILE} "#include \"${HEADER_FILE}\"\n")
    endforeach()

    file(APPEND ${MOCKS_HEADER_FILE} "#undef ENABLE_MOCKS_DECL\n")
    file(APPEND ${MOCKS_HEADER_FILE} "#undef ENABLE_MOCKS\n\n")

    file(APPEND ${MOCKS_HEADER_FILE} "#endif // ${LIBRARY_NAME}_H\n")


    file(WRITE ${MOCKS_SOURCE_FILE} "#define ENABLE_MOCKS\n")
    foreach(HEADER_FILE ${ARG_HEADER_FILES})
        file(APPEND ${MOCKS_SOURCE_FILE} "#include \"${HEADER_FILE}\"\n")
    endforeach()
    file(APPEND ${MOCKS_SOURCE_FILE} "#undef ENABLE_MOCKS\n")

    set_source_files_properties(${MOCKS_SOURCE_FILE} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

    add_library(${LIBRARY_NAME}_mocks ${MOCKS_SOURCE_FILE} ${MOCKS_HEADER_FILE})
    target_include_directories(${LIBRARY_NAME}_mocks PUBLIC $<TARGET_PROPERTY:${LIBRARY_NAME},INTERFACE_INCLUDE_DIRECTORIES>)

    target_precompile_headers(${LIBRARY_NAME}_mocks PUBLIC ${MOCKS_HEADER_FILE})
endfunction()

add_subdirectory(build_functions)

add_subdirectory(interfaces)
add_subdirectory(common)
add_subdirectory(umocktypes)
add_subdirectory(c_pal_ll)

if(WIN32)
    add_subdirectory(win32)
else()
    add_subdirectory(linux)
endif()

add_library(c_pal INTERFACE)

FILE(GLOB c_pal_md_files "doc/*.md")
SOURCE_GROUP(devdoc FILES ${c_pal_md_files})

if(MSVC)
    target_link_libraries(c_pal INTERFACE pal_win32 pal_ll_win32)
else()
    target_link_libraries(c_pal INTERFACE pal_linux pal_ll_linux)
endif()


if((CMAKE_GENERATOR MATCHES "Visual Studio") AND (${run_traceability}))
    #add traceability custom target
    add_custom_target(c_pal_traceability ALL
        COMMAND traceabilitytool -buildcheck -e ${CMAKE_CURRENT_LIST_DIR}/deps -i ${CMAKE_CURRENT_LIST_DIR})
    add_dependencies(c_pal_traceability traceabilitytool)
endif()

add_library(c_pal_reals INTERFACE)
if(MSVC)
    target_link_libraries(c_pal_reals INTERFACE win32_reals win32_ll_reals)
else()
    target_link_libraries(c_pal_reals INTERFACE linux_reals linux_ll_reals)
endif()

if(${run_reals_check})
    add_reals_check_target()
endif()

# make an install target so we can produce a Linux native client package.
# ensure the interface and common headers are a part of the installation.
FILE(GLOB c_pal_interace_includes "${CMAKE_CURRENT_LIST_DIR}/interfaces/inc/c_pal/*")
FILE(GLOB c_pal_common_includes "${CMAKE_CURRENT_LIST_DIR}/common/inc/c_pal/*")
FILE(GLOB c_pal_ll_includes "${CMAKE_CURRENT_LIST_DIR}/c_pal_ll/interfaces/inc/c_pal/*")
install_library_with_prefix(c_pal c_pal  ${c_pal_interace_includes} ${c_pal_common_includes} ${c_pal_ll_includes} )

include(CMakePackageConfigHelpers)

#Insert vld in all executables if so required
add_vld_if_defined(${CMAKE_CURRENT_SOURCE_DIR})