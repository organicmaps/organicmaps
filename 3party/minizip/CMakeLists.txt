project(minizip)

# This target is a thin C++ wrapper (unzip::/zip:: namespaces, see minizip.hpp)
# around minizip-ng's classic "compat" API. minizip-ng lives in the submodule
# 3party/minizip-ng; here we compile only the sources needed for ZIP read/write
# with DEFLATE (via zlib-ng), i.e. no encryption and no extra codecs
# (bzip2/lzma/zstd/libcompression). Building the sources directly -- instead of
# add_subdirectory(minizip-ng) -- lets us link our own zlib-ng unconditionally
# (minizip-ng's own CMake would find_package the *system* zlib, which is absent
# on Windows and would defeat the whole replacement).

set(MZNG "${OMIM_ROOT}/3party/minizip-ng")

# minizip-ng probes a few platform capabilities into a generated mz_config.h.
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
check_include_file(dirent.h     HAVE_DIRENT_H)
check_include_file(sys/dirent.h HAVE_SYS_DIRENT_H)
check_include_file(inttypes.h   HAVE_INTTYPES_H)
check_include_file(stdint.h     HAVE_STDINT_H)
check_function_exists(fseeko    HAVE_FSEEKO)
check_function_exists(symlink   HAVE_SYMLINK)
check_function_exists(readlink  HAVE_READLINK)
if (HAVE_DIRENT_H)
  list(APPEND CMAKE_EXTRA_INCLUDE_FILES dirent.h)
endif()
if (HAVE_SYS_DIRENT_H)
  list(APPEND CMAKE_EXTRA_INCLUDE_FILES sys/dirent.h)
endif()
check_type_size("DIR*" PDIR)
unset(CMAKE_EXTRA_INCLUDE_FILES)
configure_file(${MZNG}/mz_config.h.cmakein ${CMAKE_CURRENT_BINARY_DIR}/mz_config.h @ONLY NEWLINE_STYLE UNIX)

# minizip-ng is compiled through a single amalgamation TU (minizip_ng.c, which
# #includes the required sources). This keeps the CMake and Xcode builds on one
# source list -- the Xcode project references the same minizip_ng.c.
set(SRC
  minizip.cpp
  minizip.hpp
  minizip_ng.c
)

add_library(${PROJECT_NAME} ${SRC})

# minizip_ng.c is a self-contained amalgamation TU; keep it and the C++ wrapper
# out of OM's cross-file unity batches so it compiles exactly like the Xcode build.
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD OFF)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${MZNG}
  ${CMAKE_CURRENT_BINARY_DIR}  # generated mz_config.h
)

# The minizip-ng feature defines (HAVE_ZLIB / ZLIB_COMPAT / MZ_ZIP_NO_CRYPTO /
# MZ_ZIP_NO_ENCRYPTION) are set at the top of minizip_ng.c, so the CMake and
# Xcode builds share one source of truth.

target_compile_options(${PROJECT_NAME} PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wno-unused-value>)

target_link_libraries(${PROJECT_NAME} PUBLIC ZLIB::ZLIB)
