#pragma once

#include "base/assert.hpp"

#include <bitset>
#include <initializer_list>
#include <string>

namespace routing::turns::lanes
{
enum class LaneWay : std::uint8_t
{
  None = 0,
  Left,
  SlightLeft,
  SharpLeft,
  Through,
  Right,
  SlightRight,
  SharpRight,
  Reverse,
  MergeToLeft,
  MergeToRight,

  Count
};

class LaneWays
{
  using LaneWaysT = std::bitset<static_cast<std::uint8_t>(LaneWay::Count)>;

  friend std::string DebugPrint(LaneWays const & laneWays);

public:
  constexpr LaneWays() = default;
  constexpr LaneWays(std::initializer_list<LaneWay> const laneWays)
  {
    for (auto const & laneWay : laneWays)
      Add(laneWay);
  }

  constexpr bool operator==(LaneWays const & rhs) const { return m_laneWays == rhs.m_laneWays; }

  constexpr void Add(LaneWay laneWay)
  {
    ASSERT_LESS(laneWay, LaneWay::Count, ());
    m_laneWays.set(static_cast<std::uint8_t>(laneWay));
  }

  constexpr bool Contains(LaneWay laneWay) const
  {
    ASSERT_LESS(laneWay, LaneWay::Count, ());
    return m_laneWays.test(static_cast<std::uint8_t>(laneWay));
  }

private:
  LaneWaysT m_laneWays;
};

std::string DebugPrint(LaneWay laneWay);
std::string DebugPrint(LaneWays const & laneWays);
}  // namespace routing::turns::lanes
