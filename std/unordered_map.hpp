#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/unordered_map.hpp>
using boost::unordered_map;
using boost::unordered_multimap;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
