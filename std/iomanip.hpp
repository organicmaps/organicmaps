#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <iomanip>
using std::setw;
using std::setfill;
using std::hex;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
