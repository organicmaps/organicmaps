#pragma once

#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

// for errno
#ifdef OMIM_OS_WINDOWS
  #include <errno.h>
#else
  #include <sys/errno.h>
#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
