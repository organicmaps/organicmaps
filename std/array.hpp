#pragma once

#ifdef new
#undef new
#endif

#include <array>

using std::array;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
