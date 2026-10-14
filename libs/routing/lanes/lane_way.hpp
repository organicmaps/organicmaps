#pragma once

#include "base/assert.hpp"

#include <bitset>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace routing::turns::lanes
{
enum class LaneWay : std::uint8_t
{
  None = 0,
  ReverseLeft,
  SharpLeft,
  Left,
  MergeToLeft,
  SlightLeft,
  Through,
  SlightRight,
  MergeToRight,
  Right,
  SharpRight,
  ReverseRight,

  Count
};

class LaneWays
{
  friend std::string DebugPrint(LaneWays const & laneWays);

public:
  constexpr LaneWays() = default;
  constexpr LaneWays(std::initializer_list<LaneWay> const laneWays)
  {
    for (auto const & laneWay : laneWays)
      Add(laneWay);
  }

  constexpr bool operator==(LaneWays const & rhs) const { return m_bits == rhs.m_bits; }

  constexpr void Add(LaneWay laneWay)
  {
    ASSERT_LESS(laneWay, LaneWay::Count, ());
    m_bits |= Mask(laneWay);
  }

  constexpr void Remove(LaneWay laneWay)
  {
    ASSERT_LESS(laneWay, LaneWay::Count, ());
    m_bits &= ~Mask(laneWay);
  }

  constexpr bool Contains(LaneWay laneWay) const
  {
    ASSERT_LESS(laneWay, LaneWay::Count, ());
    return (m_bits & Mask(laneWay)) != 0;
  }

  /// An unrestricted lane is a lane that has no restrictions, i.e., it contains no lane ways.
  constexpr bool IsUnrestricted() const
  {
    return m_bits == 0 || (PopCount(m_bits) == 1 && Contains(LaneWay::None));
  }

  [[nodiscard]] std::vector<LaneWay> GetActiveLaneWays() const
  {
    std::vector<LaneWay> result;
    for (std::size_t i = 0; i < static_cast<std::size_t>(LaneWay::Count); ++i)
      if (Contains(static_cast<LaneWay>(i)))
        result.emplace_back(static_cast<LaneWay>(i));
    return result;
  }

private:
  // std::bitset is only constexpr since C++23, which GCC 12 (Aurora OS) does not provide,
  // so a plain bit mask is used instead.
  using BitsT = std::uint16_t;
  static constexpr BitsT Mask(LaneWay laneWay)
  {
    return BitsT{1} << static_cast<std::uint8_t>(laneWay);
  }
  static constexpr int PopCount(BitsT bits)
  {
    int count = 0;
    while (bits != 0)
    {
      count += bits & 1;
      bits >>= 1;
    }
    return count;
  }

  BitsT m_bits = 0;
};

std::string DebugPrint(LaneWay laneWay);
std::string DebugPrint(LaneWays const & laneWays);
}  // namespace routing::turns::lanes
