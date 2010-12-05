#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/type_traits.hpp>
using boost::is_same;
using boost::make_signed;
using boost::make_unsigned;
using boost::is_signed;
using boost::is_unsigned;
using boost::is_floating_point;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
