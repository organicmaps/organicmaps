#pragma once

#ifdef new
#undef new
#endif

#include <sstream>

using std::istringstream;
using std::ostringstream;
using std::stringstream;
using std::endl;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
