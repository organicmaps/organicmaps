#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <bitset>
using std::bitset;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
