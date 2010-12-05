#pragma once

#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/timer.hpp>
//using boost::timer; do not use word timer, because it's very common

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
