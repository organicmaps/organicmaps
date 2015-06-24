#pragma once

#include "target_os.hpp"
#include "ctime.hpp"

#ifdef new
#undef new
#endif

// for gettimeofday and GetSystemTimeAsFileTime
#ifdef OMIM_OS_WINDOWS
  #include "windows.hpp"
  #ifdef OMIM_OS_WINDOWS_MINGW
    #define gettimeofday mingw_gettimeofday
  #endif
#else
  #include <sys/time.h>
#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
