#pragma once

#include <cstdint>
#include <vector>

namespace routing
{
// This class provides information about road access classes.
// For now, only restrictive types (such as barrier=gate and access=private)
// and only car routing are supported.
class RoadAccess
{
public:
  RoadAccess() = default;

  std::vector<uint32_t> & GetPrivateRoads() { return m_privateRoads; }
  std::vector<uint32_t> const & GetPrivateRoads() const { return m_privateRoads; }

  void Clear() { m_privateRoads.clear(); }

  bool operator==(RoadAccess const & rhs) { return m_privateRoads == rhs.m_privateRoads; }

private:
  // Feature ids of blocked features in the corresponding mwm.
  std::vector<uint32_t> m_privateRoads;
};
}  // namespace routing
