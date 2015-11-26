#pragma once

#ifdef new
#undef new
#endif

#include <chrono>

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::hours;
using std::chrono::milliseconds;
using std::chrono::minutes;
using std::chrono::nanoseconds;
using std::chrono::seconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;
using std::chrono::time_point;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
