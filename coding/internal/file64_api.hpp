#pragma once

#include "base/base.hpp"

#if defined(OMIM_OS_WINDOWS_NATIVE)
  #define fseek64 _fseeki64
  #define ftell64 _ftelli64

#elif defined(OMIM_OS_TIZEN)
  static_assert(sizeof(__off64_t) == 8, "");
  #define fseek64 fseeko64
  #define ftell64 ftello64

  #define __LARGE64_FILES
  typedef uint64_t _fpos64_t;

#elif defined(OMIM_OS_WINDOWS_MINGW)
  #define fseek64 fseeko64
  #define ftell64 ftello64

#else
  // POSIX standart.
  #include <sys/types.h>
  #ifdef OMIM_OS_ANDROID
    static_assert(sizeof(off_t) == 4, "");
  #else
    static_assert(sizeof(off_t) == 8, "");
  #endif
  #define fseek64 fseeko
  #define ftell64 ftello

#endif

#include "std/cstdio.hpp"
