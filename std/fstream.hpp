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

  using std::ofstream;
  using std::ifstream;
#endif

using std::ios;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
