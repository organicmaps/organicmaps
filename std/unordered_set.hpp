#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/unordered_set.hpp>
using boost::unordered_set;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
