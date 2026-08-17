#ifndef MZ_CONFIG_H
#define MZ_CONFIG_H

// Hand-maintained minizip-ng configuration for the Apple (iOS/macOS) Xcode
// build, which cannot run minizip-ng's CMake to generate this header. All of
// these capabilities are available on Apple platforms. The CMake build
// generates the equivalent file from 3party/minizip-ng/mz_config.h.cmakein.

#define HAVE_DIRENT_H     1
#define HAVE_SYS_DIRENT_H 1
#define HAVE_INTTYPES_H   1
#define HAVE_STDINT_H     1
#define HAVE_PDIR         1
#define HAVE_FSEEKO       1
#define HAVE_SYMLINK      1
#define HAVE_READLINK     1

#endif
