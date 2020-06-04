#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ctest" for configuration ""
set_property(TARGET ctest APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(ctest PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "azure_c_logging"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libctest.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS ctest )
list(APPEND _IMPORT_CHECK_FILES_FOR_ctest "${_IMPORT_PREFIX}/lib/libctest.a" )

# Import target "azure_c_logging" for configuration ""
set_property(TARGET azure_c_logging APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(azure_c_logging PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libazure_c_logging.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS azure_c_logging )
list(APPEND _IMPORT_CHECK_FILES_FOR_azure_c_logging "${_IMPORT_PREFIX}/lib/libazure_c_logging.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
