#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef OMIM_USE_DEBUG_STL
  #include <debug/list>
#else
  #include <list>
#endif
using std::list;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
