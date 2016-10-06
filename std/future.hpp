#pragma once

#ifdef new
#undef new
#endif

#include <future>

using std::async;
using std::future;
using std::future_status;
using std::launch;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
