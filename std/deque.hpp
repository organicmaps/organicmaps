#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#if defined(DEBUG) && (defined(OMIM_OS_LINUX) || defined(OMIM_OS_MAC))
  #include <debug/deque>
#else
  #include <deque>
#endif
using std::deque;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
