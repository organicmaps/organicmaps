#pragma once

#ifdef new
#undef new
#endif

#include <deque>
using std::deque;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
