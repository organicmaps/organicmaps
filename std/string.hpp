#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#if defined(DEBUG) && (defined(OMIM_OS_LINUX) || defined(OMIM_OS_MAC))
  #include <debug/string>
#else
  #include <string>
#endif

using std::basic_string;
using std::string;
using std::getline;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
