#pragma once

#ifdef new
#undef new
#endif

#include <ctime>

using std::time;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
