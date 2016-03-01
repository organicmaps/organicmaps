#pragma once

#include "std/cstdint.hpp"

namespace search
{
namespace v2
{
struct PreRankingInfo
{
  static double const kMaxDistMeters;

  // Distance from the mwm center to the current viewport's center.
  double m_mwmDistanceToViewport = kMaxDistMeters;

  // Distance from the feature to the current user's position.
  double m_mwmDistanceToPosition = kMaxDistMeters;

  // Tokens [m_startToken, m_endToken) match to the query.
  size_t m_startToken = 0;
  size_t m_endToken = 0;

  // Rank of the feature.
  uint8_t m_rank = 0;
};
}  // namespace v2
}  // namespace search
