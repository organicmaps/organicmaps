#pragma once

#include "std/target_os.hpp"

#if defined(OMIM_OS_WINDOWS_NATIVE)
#define fseek64 _fseeki64
#define ftell64 _ftelli64

#elif defined(OMIM_OS_WINDOWS_MINGW)
#define fseek64 fseeko64
#define ftell64 ftello64

#else
// POSIX standart.
#include <sys/types.h>

// TODO: Always assert for 8 bytes after increasing min Android API to 24+.
// See more details here: https://android.googlesource.com/platform/bionic/+/master/docs/32-bit-abi.md
#if defined(OMIM_OS_ANDROID) && (defined(__arm__) || defined(__i386__))
static_assert(sizeof(off_t) == 4, "32-bit Android NDK < API 24 has only 32-bit file operations support");
#else
static_assert(sizeof(off_t) == 8, "FileReader and FileWriter require 64-bit file operations");
#endif
#define fseek64 fseeko
#define ftell64 ftello

#endif

#include <cstdio>
