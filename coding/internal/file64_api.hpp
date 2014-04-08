#pragma once

#include "../../base/base.hpp"

#if defined(OMIM_OS_WINDOWS_NATIVE)
  #define fseek64 _fseeki64
  #define ftell64 _ftelli64

#elif defined(OMIM_OS_TIZEN)
  STATIC_ASSERT(sizeof(__off64_t) == 8);
  #define fseek64 fseeko64
  #define ftell64 ftello64

  #define __LARGE64_FILES
  typedef uint64_t _fpos64_t;

#elif defined(OMIM_OS_WINDOWS_MINGW)
  //STATIC_ASSERT(sizeof(off64_t) == 8);
  #define fseek64 fseeko64
  #define ftell64 ftello64

#elif defined(OMIM_OS_ANDROID)
  // For Android, off_t is 32bit, so big files are not supported
  STATIC_ASSERT(sizeof(off_t) == 4);
  #define fseek64 fseeko
  #define ftell64 ftello

#else
  STATIC_ASSERT(sizeof(off_t) == 8);
  #define fseek64 fseeko
  #define ftell64 ftello

#endif

#include "../../std/cstdio.hpp"
