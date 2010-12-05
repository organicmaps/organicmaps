#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <set>
using std::set;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
