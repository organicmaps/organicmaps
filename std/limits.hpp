#pragma once

#ifdef new
#undef new
#endif

#include <climits>
#include <limits>
using std::numeric_limits;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
