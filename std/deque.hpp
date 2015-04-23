#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef OMIM_USE_DEBUG_STL
  #include <debug/deque>
#else
  #include <deque>
#endif
using std::deque;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
