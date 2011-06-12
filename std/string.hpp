#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#include <string>

using std::basic_string;
using std::string;
using std::getline;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
