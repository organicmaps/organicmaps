#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef OMIM_USE_DEBUG_STL
  #include <debug/bitset>
#else
  #include <bitset>
#endif
using std::bitset;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
