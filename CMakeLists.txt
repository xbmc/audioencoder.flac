cmake_minimum_required(VERSION 3.5)
project(audioencoder.flac)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(FLAC REQUIRED)
find_package(Ogg REQUIRED)

include_directories(${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)
                    ${FLAC_INCLUDE_DIRS}
                    ${OGG_INCLUDE_DIRS})

set(FLAC_SOURCES src/EncoderFlac.cpp)

set(DEPLIBS ${FLAC_LIBRARIES} ${OGG_LIBRARIES})

# on win32 we use a static version of libflac so we need to define FLAC__NO_DLL
if(WIN32)
  add_definitions(-DFLAC__NO_DLL)
endif()

build_addon(audioencoder.flac FLAC DEPLIBS)

include(CPack)
