#pragma once

#ifdef new
#undef new
#endif

#include <numeric>
using std::accumulate;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
