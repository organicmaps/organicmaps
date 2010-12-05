#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef _MSC_VER
  typedef __int8            int8_t;
  typedef __int16           int16_t;
  typedef __int32           int32_t;
  typedef __int64           int64_t;
  typedef unsigned __int8   uint8_t;
  typedef unsigned __int16  uint16_t;
  typedef unsigned __int32  uint32_t;
  typedef unsigned __int64  uint64_t;

#elif defined(OMIM_OS_BADA)
  #include <stdint.h>

#else
  #include <boost/cstdint.hpp>

#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
