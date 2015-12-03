#pragma once

#ifdef new
#undef new
#endif

#include <fstream>
using std::ofstream;
using std::ifstream;
using std::fstream;
using std::ios;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
