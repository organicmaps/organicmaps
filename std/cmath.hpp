#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <cmath>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif

namespace math
{
  static const double pi = 3.14159265358979323846;

  template <class T> T sqr(T t) { return (t*t); }
}
