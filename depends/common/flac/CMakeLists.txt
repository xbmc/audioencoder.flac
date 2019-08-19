cmake_minimum_required(VERSION 3.5)
project(flac)

include(TestCXXAcceptsFlag)

CHECK_CXX_ACCEPTS_FLAG("-fPIC" HAVE_FPIC)
if(HAVE_FPIC)
  set(EXTRA_CFLAGS "CFLAGS=-fPIC -DPIC -O2")
endif()

include(ExternalProject)
externalproject_add(flac
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    UPDATE_COMMAND autoreconf -vif
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure 
                      --prefix=${OUTPUT_DIR}
                      --disable-xmms-plugin
                      --disable-cpplibs
                      --disable-asm-optimizations
                      --enable-static
                      --disable-shared
                      ${EXTRA_CFLAGS}
                    INSTALL_COMMAND ""
                    BUILD_IN_SOURCE 1)

install(CODE "execute_process(COMMAND make install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
