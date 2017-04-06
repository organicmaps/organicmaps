#pragma once

#ifdef new
#undef new
#endif

#include <iostream>

using std::cin;
using std::cout;
using std::cerr;

using std::istream;
using std::ostream;

using std::ios_base;

using std::endl;
using std::flush;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
