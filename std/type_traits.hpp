#pragma once

#ifdef new
#undef new
#endif

#include <type_traits>

using std::conditional;
using std::enable_if;
using std::is_arithmetic;
using std::is_base_of;
using std::is_constructible;
using std::is_convertible;
using std::is_enum;
using std::is_floating_point;
using std::is_integral;
using std::is_pod;
using std::is_same;
using std::is_signed;
using std::is_standard_layout;
using std::is_unsigned;
using std::is_void;
using std::make_signed;
using std::make_unsigned;
using std::remove_reference;
using std::underlying_type;

using std::result_of;

using std::false_type;
using std::true_type;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
