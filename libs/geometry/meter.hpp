#pragma once

namespace m2
{
using Meter = double;

inline namespace literals
{
inline constexpr Meter operator"" _m(long double value) noexcept
{
  return static_cast<double>(value);
}

inline constexpr Meter operator"" _km(long double value) noexcept
{
  return static_cast<double>(value * 1000.0);
}
}  // namespace literals
}  // namespace m2
