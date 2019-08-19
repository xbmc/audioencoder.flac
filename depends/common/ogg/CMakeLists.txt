cmake_minimum_required(VERSION 3.5)
project(ogg)

include(CheckCCompilerFlag)
include(ExternalProject)

externalproject_add(ogg
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    UPDATE_COMMAND autoreconf -vif <SOURCE_DIR>
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${OUTPUT_DIR}
                      --enable-static
                      --disable-shared
                      "CFLAGS=-fPIC -DPIC -O2"
                    INSTALL_COMMAND ""
                    BUILD_IN_SOURCE 1)
                  
install(CODE "execute_process(COMMAND make install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
