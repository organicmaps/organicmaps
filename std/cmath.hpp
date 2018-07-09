#pragma once

#ifdef new
#undef new
#endif

// We do not define _USE_MATH_DEFINES - please add your constants below.
#include <cmath>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif

using std::abs;
using std::cos;
using std::isfinite;
using std::sin;
using std::log10;
