#pragma once

#include <cstdint>
#include <string>

namespace traffic
{
enum class SpeedGroup : uint8_t
{
  G0 = 0,
  G1,
  G2,
  G3,
  G4,
  G5,
  TempBlock,
  Unknown,
  Count
};

static_assert(static_cast<uint8_t>(SpeedGroup::Count) <= 8, "");

// Let M be the maximal speed that is possible on a free road
// and let V be the maximal speed that is possible on this road when
// taking the traffic data into account.
// We group all possible ratios (V/M) into a small number of
// buckets and only use the number of a bucket everywhere.
// That is, we forget the specific values of V when transmitting and
// displaying traffic information. The value M of a road is known at the
// stage of building the mwm containing this road.
//
// kSpeedGroupThresholdPercentage[g] denotes the maximal value of (V/M)
// that is possible for group |g|. Values falling on a border of two groups
// may belong to either group.
//
// The threshold percentage is defined to be 100 for the
// special groups where V is unknown or not defined.
extern uint32_t const kSpeedGroupThresholdPercentage[static_cast<size_t>(SpeedGroup::Count)];

/// \note This method is used in traffic jam generation.
SpeedGroup GetSpeedGroupByPercentage(double p);

std::string DebugPrint(SpeedGroup const & group);
}  // namespace traffic
