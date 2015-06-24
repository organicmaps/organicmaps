#pragma once

#ifdef new
#undef new
#endif

// We do not define _USE_MATH_DEFINES - please add your constants below.
#include <cmath>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif

namespace math
{
  static const double pi = 3.14159265358979323846;
  static const double pi2 = pi / 2.;
  static const double pi4 = pi / 4.;
  static const double twicePi = 2. * pi;

  template <class T> T sqr(T t) { return (t*t); }
}
