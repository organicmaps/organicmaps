#pragma once

#ifdef new
#undef new
#endif

#include <complex>

using std::complex;
using std::polar;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
