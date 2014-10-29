project(audioencoder.flac)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)
find_package(FLAC REQUIRED)
find_package(Ogg REQUIRED)

include_directories(${KODI_INCLUDE_DIR}
                    ${FLAC_INCLUDE_DIRS}
                    ${OGG_INCLUDE_DIRS})

set(FLAC_SOURCES src/EncoderFlac.cpp)

set(DEPLIBS ${FLAC_LIBRARIES} ${OGG_LIBRARIES})

# on win32 we use a static version of libflac so we need to define FLAC__NO_DLL
if(WIN32)
  add_definitions(-DFLAC__NO_DLL)
endif()

build_addon(audioencoder.flac FLAC DEPLIBS)
