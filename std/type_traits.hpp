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

#include <type_traits>

using std::enable_if;
using std::conditional;

using std::is_same;
using std::make_signed;
using std::make_unsigned;
using std::is_signed;
using std::is_unsigned;
using std::is_floating_point;
using std::is_integral;
using std::is_arithmetic;
using std::is_pod;
using std::underlying_type;

/// @todo clang on linux doesn't have is_trivially_copyable.
#ifndef OMIM_OS_LINUX
using std::is_trivially_copyable;
#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
