#pragma once

#include "platform/location.hpp"

#include <ctime>
#include <optional>

namespace location
{
class PositionFilter
{
public:
  bool Passes(GpsInfo const & info, std::time_t now = std::time(nullptr))
  {
    // Provider timestamps follow the system clock; a backward correction invalidates the ordering reference.
    if (now < m_lastSystemTime)
      m_timestamp.reset();
    m_lastSystemTime = now;

    if (now - info.m_timestamp > 300.0 || (m_timestamp && info.m_timestamp < *m_timestamp))
      return false;
    m_timestamp = info.m_timestamp;
    return true;
  }

private:
  std::optional<double> m_timestamp;
  std::time_t m_lastSystemTime = 0;
};
}  // namespace location
