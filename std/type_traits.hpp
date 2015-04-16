#pragma once
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#include <type_traits>

using std::conditional;
using std::enable_if;
using std::is_arithmetic;
using std::is_floating_point;
using std::is_integral;
using std::is_pod;
using std::is_same;
using std::is_signed;
using std::is_unsigned;
using std::make_signed;
using std::make_unsigned;
using std::underlying_type;
using std::is_same;
using std::is_signed;
using std::is_standard_layout;
using std::is_unsigned;
using std::make_signed;
using std::make_unsigned;
using std::is_void;

/// @todo clang on linux doesn't have is_trivially_copyable.
#ifndef OMIM_OS_LINUX
using std::is_trivially_copyable;
#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
