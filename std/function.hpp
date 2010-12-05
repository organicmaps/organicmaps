#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/function.hpp>
using boost::function;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
