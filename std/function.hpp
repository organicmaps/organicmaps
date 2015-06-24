#pragma once

#ifdef new
#undef new
#endif

#include <functional>
using std::function;
using std::greater;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
