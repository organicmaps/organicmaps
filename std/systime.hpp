#pragma once

#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

// for gettimeofday and GetSystemTimeAsFileTime
#ifdef OMIM_OS_WINDOWS
  #include "windows.hpp"
#else
  #include <sys/time.h>
#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
