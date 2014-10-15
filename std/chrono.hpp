#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::nanoseconds;
using std::chrono::duration_cast;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
