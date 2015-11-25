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

namespace math
{
  double constexpr pi = 3.14159265358979323846;
  double constexpr pi2 = pi / 2.;
  double constexpr pi4 = pi / 4.;
  double constexpr twicePi = 2. * pi;

  template <class T> T sqr(T t) { return (t*t); }
}
