# - Try to find FLAC
# Once done this will define
#
# FLAC_FOUND - system has libFLAC
# FLAC_INCLUDE_DIRS - the libFLAC include directory
# FLAC_LIBRARIES - The libFLAC libraries

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_FLAC flac QUIET)
endif()

find_path(FLAC_INCLUDE_DIRS FLAC/stream_decoder.h
                            PATHS ${PC_FLAC_INCLUDEDIR})
find_library(FLAC_LIBRARIES NAMES FLAC libFLAC_static
                            PATHS ${PC_FLAC_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLAC REQUIRED_VARS FLAC_INCLUDE_DIRS FLAC_LIBRARIES)

mark_as_advanced(FLAC_INCLUDE_DIRS FLAC_LIBRARIES)
