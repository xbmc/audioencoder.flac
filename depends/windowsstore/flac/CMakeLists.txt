project(flac)

cmake_minimum_required(VERSION 2.8)

include(CheckSymbolExists)
check_symbol_exists(_X86_ "Windows.h" _X86_)
check_symbol_exists(_AMD64_ "Windows.h" _X64_)
check_symbol_exists(_ARM_ "Windows.h" _ARM_)

find_program(NASM_COMPILER NAMES nasm nasmw)
add_definitions(-DPACKAGE_VERSION="1.3.2"
                -DFLAC__NO_DLL -DFLAC__ALIGN_MALLOC_DATA -DFLAC__HAS_OGG -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS
                -Dfseeko=_fseeki64 -Dftello=_ftelli64) # work around a bug in the code that leads to "undefined reference errors"

if(NOT _ARM_)
    add_definitions(-DFLAC__SSE_OS -DFLAC__HAS_X86INTRIN)
endif()

if(_X86_)
  add_definitions(-DFLAC__CPU_IA32 -DFLAC__HAS_NASM)
elseif(_X64_)
  add_definitions(-DENABLE_64_BIT_WORDS -DFLAC__CPU_X86_64)
endif()

# function to setup a custom command for a compiled assembler file
function (build_nasm filename_we)
  add_custom_command(OUTPUT src/libFLAC/ia32/${filename_we}.obj
                     COMMAND ${NASM_COMPILER} -f win32 -d OBJ_FORMAT_win32 -i ${PROJECT_SOURCE_DIR}/src/libFLAC/ia32/ ${PROJECT_SOURCE_DIR}/src/libFLAC/ia32/${filename_we}.nasm -o ${PROJECT_BINARY_DIR}/src/libFLAC/ia32/${filename_we}.obj
                     WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endfunction (build_nasm filename)

# make sure that the output directory for the compiled assembler files exists
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src/libFLAC/ia32)
  
# setup custom commands for the compiled assembler files
set(LIBFLAC_SOURCES src/libFLAC/bitmath.c
                    src/libFLAC/bitreader.c
                    src/libFLAC/bitwriter.c
                    src/libFLAC/cpu.c
                    src/libFLAC/crc.c
                    src/libFLAC/fixed.c
                    src/libFLAC/fixed_intrin_sse2.c
                    src/libFLAC/fixed_intrin_ssse3.c
                    src/libFLAC/float.c
                    src/libFLAC/format.c
                    src/libFLAC/lpc.c
                    src/libFLAC/lpc_intrin_avx2.c
                    src/libFLAC/lpc_intrin_sse.c
                    src/libFLAC/lpc_intrin_sse2.c
                    src/libFLAC/lpc_intrin_sse41.c
                    src/libFLAC/md5.c
                    src/libFLAC/memory.c
                    src/libFLAC/metadata_iterators.c
                    src/libFLAC/metadata_object.c
                    src/libFLAC/ogg_decoder_aspect.c
                    src/libFLAC/ogg_encoder_aspect.c
                    src/libFLAC/ogg_helper.c
                    src/libFLAC/ogg_mapping.c
                    src/libFLAC/stream_decoder.c
                    src/libFLAC/stream_encoder.c
                    src/libFLAC/stream_encoder_framing.c
                    src/libFLAC/stream_encoder_intrin_avx2.c
                    src/libFLAC/stream_encoder_intrin_sse2.c
                    src/libFLAC/stream_encoder_intrin_ssse3.c
                    src/libFLAC/window.c
                    src/libFLAC/windows_unicode_filenames.c)

if(_X86_)
  build_nasm(bitreader_asm)
  build_nasm(cpu_asm)
  build_nasm(fixed_asm)
  build_nasm(lpc_asm)
  build_nasm(stream_encoder_asm)

  list(APPEND LIBFLAC_SOURCES
                    src/libFLAC/ia32/cpu_asm.obj
                    src/libFLAC/ia32/fixed_asm.obj
                    src/libFLAC/ia32/lpc_asm.obj)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src/libFLAC/include ${CMAKE_INSTALL_PREFIX}/include)

add_library(flac ${LIBFLAC_SOURCES})

set(HEADERS ${PROJECT_SOURCE_DIR}/include/FLAC/all.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/assert.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/callback.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/export.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/format.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/metadata.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/ordinals.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/stream_decoder.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/stream_encoder.h)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/FLAC)
install(TARGETS flac DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
