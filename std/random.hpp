#pragma once

#ifdef new
#undef new
#endif

#include <random>

using std::default_random_engine;
using std::minstd_rand;
using std::mt19937;
using std::uniform_int_distribution;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
