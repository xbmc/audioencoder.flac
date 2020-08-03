cmake_minimum_required(VERSION 3.5)
project(audioencoder.flac)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

include(ExternalProject)

find_program(PATCH_EXECUTABLE NAMES patch)

if(NOT PATCH_EXECUTABLE)
  message(FATAL_ERROR "Missing patch command (we looked in ${CMAKE_PREFIX_PATH})")
endif()

ExternalProject_Add(audioencoder.flac
  URL https://github.com/xiph/ogg/archive/v1.3.4.tar.gz
  URL_HASH SHA256=3da31a4eb31534b6f878914b7379b873c280e610649fe5c07935b3d137a828bc
  PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_CURRENT_SOURCE_DIR}/0001-fix-Mac-with-error-unknown-type-name-uint64_t.patch
  INSTALL_COMMAND     ""
)
