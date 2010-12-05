#pragma once

#include "../../base/base.hpp"

#ifdef _MSC_VER
  #define fseek64 _fseeki64
  #define ftell64 _ftelli64

#elif defined(OMIM_OS_BADA)
  STATIC_ASSERT(sizeof(_off64_t) == 8);
  #define fseek64 fseeko64
  #define ftell64 ftello64

  #define __LARGE64_FILES
  typedef uint64_t _fpos64_t;

#elif defined(OMIM_OS_WINDOWS)
  STATIC_ASSERT(sizeof(off64_t) == 8);
  #define fseek64 fseeko64
  #define ftell64 ftello64

#else
  STATIC_ASSERT(sizeof(off_t) == 8);
  #define fseek64 fseeko
  #define ftell64 ftello

#endif

#include "../../std/stdio.hpp"
