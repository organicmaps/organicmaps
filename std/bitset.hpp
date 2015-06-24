#pragma once

#ifdef new
#undef new
#endif

#include <bitset>
using std::bitset;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
