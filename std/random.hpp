#pragma once

#ifdef new
#undef new
#endif

#include <random>

using std::mt19937;
using std::uniform_int_distribution;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
