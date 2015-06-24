#pragma once

#ifdef new
#undef new
#endif

#include <atomic>

using std::atomic;
using std::atomic_flag;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
