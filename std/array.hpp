#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/array.hpp>

using boost::array;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
