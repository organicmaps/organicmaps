#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/shared_ptr.hpp>
using boost::shared_ptr;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
