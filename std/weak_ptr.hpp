#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/weak_ptr.hpp>
using boost::weak_ptr;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif