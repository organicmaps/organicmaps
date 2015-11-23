#pragma once

#include "std/ctime.hpp"
#include "std/string.hpp"

namespace base
{

inline bool NotATime(time_t const t) noexcept { return t == -1; }

// See http://www.w3.org/TR/NOTE-datetime to learn more
// about the format.

// Parses a string of a format YYYY-MM-DDThh:mmTZD
// (eg 1997-07-16T19:20:30.45+01:00).
// Returns timestamp corresponding to w3ctime or -1 on failure.
time_t ParseTime(std::string const & w3ctime) noexcept;

// Converts timestamp to a string of a fromat YYYY-MM-DDThh:mmTZD
// or empty string if cannot convert.
std::string TimeToString(time_t const timestamp);
} // namespace base
