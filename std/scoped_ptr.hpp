#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/scoped_ptr.hpp>
using boost::scoped_ptr;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
