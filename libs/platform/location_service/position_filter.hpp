#pragma once

#include "platform/location.hpp"

#include <optional>

namespace location
{
// Provider timestamps can be ahead of the device clock. Only age and backwards order are filtered.
inline bool IsCurrentLocation(GpsInfo const & info, std::optional<GpsInfo> const & previous, double nowSec)
{
  double constexpr kMaxLocationAgeSec = 300;
  return nowSec - info.m_timestamp <= kMaxLocationAgeSec && (!previous || info.m_timestamp >= previous->m_timestamp);
}
}  // namespace location
