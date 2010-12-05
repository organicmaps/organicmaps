#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <ios>

using std::ios_base;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
