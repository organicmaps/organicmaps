#pragma once

#ifdef new
#undef new
#endif

#include <numeric>
using std::accumulate;
using std::iota;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
