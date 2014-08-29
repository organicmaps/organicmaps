#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef OMIM_OS_MAC
// http://stackoverflow.com/questions/8173620/c-boost-1-48-type-traits-and-cocoa-inclusion-weirdness
// Cocoa defines "check" macros as is. It breaks compilation of boost/type_traits.
#ifdef check
#undef check
#endif
#endif

#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>

using boost::enable_if;

using boost::is_same;
using boost::make_signed;
using boost::make_unsigned;
using boost::is_signed;
using boost::is_unsigned;
using boost::is_floating_point;
using boost::is_integral;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
