#pragma once

#ifdef new
#undef new
#endif

#include <boost/shared_array.hpp>
using boost::shared_array;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
