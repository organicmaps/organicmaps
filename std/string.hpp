#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef OMIM_USE_DEBUG_STL
  #include <debug/string>
#else
  #include <string>
#endif

using std::basic_string;
using std::getline;
using std::stoi;
using std::string;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
