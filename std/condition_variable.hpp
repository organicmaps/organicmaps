#pragma once

#ifdef new
#undef new
#endif

#include <condition_variable>

using std::condition_variable;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
