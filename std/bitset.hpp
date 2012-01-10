#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#if defined(DEBUG) && (defined(OMIM_OS_LINUX) || defined(OMIM_OS_MAC))
  #include <debug/bitset>
#else
  #include <bitset>
#endif
using std::bitset;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
