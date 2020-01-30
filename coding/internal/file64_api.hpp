#pragma once

#include "base/base.hpp"

#if defined(OMIM_OS_WINDOWS_NATIVE)
  #define fseek64 _fseeki64
  #define ftell64 _ftelli64

#elif defined(OMIM_OS_WINDOWS_MINGW)
  #define fseek64 fseeko64
  #define ftell64 ftello64

#else
  // POSIX standart.
  #include <sys/types.h>
  #ifndef OMIM_OS_ANDROID
    static_assert(sizeof(off_t) == 8, "");
  #endif
  #define fseek64 fseeko
  #define ftell64 ftello

#endif

#include <cstdio>
