#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef OMIM_OS_BADA
  #include <ios>
#else
  #include <fstream>
#endif

using std::ofstream;
using std::ostream;
using std::ifstream;
using std::istream;
using std::ios;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
