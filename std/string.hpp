#pragma once

#ifdef new
#undef new
#endif

#include <string>

using std::basic_string;
using std::getline;
using std::stoi;
using std::stod;
using std::string;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
