#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <limits>
using std::numeric_limits;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
