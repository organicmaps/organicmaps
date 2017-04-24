#pragma once

#ifdef new
#undef new
#endif

#include <iomanip>
using std::fixed;
using std::hex;
using std::left;
using std::setfill;
using std::setprecision;
using std::setw;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
