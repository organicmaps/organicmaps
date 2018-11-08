#include "routing/maxspeed_serialization.hpp"

namespace routing
{
void GetForwardMaxspeedStats(std::vector<FeatureMaxspeed> const & speeds,
                             size_t & forwardMaxspeedsNumber, uint32_t & maxForwardFeatureId)
{
  for (auto const & s : speeds)
  {
    if (!s.IsBidirectional())
    {
      ++forwardMaxspeedsNumber;
      maxForwardFeatureId = s.GetFeatureId();
    }
  }
}

void CheckSpeeds(std::vector<FeatureMaxspeed> const & speeds)
{
  CHECK(std::is_sorted(speeds.cbegin(), speeds.cend()), ());
  CHECK(std::adjacent_find(speeds.cbegin(), speeds.cend(),
                           [](auto const & lhs, auto const & rhs) {
                             return lhs.GetFeatureId() == rhs.GetFeatureId();
                           }) == speeds.cend(),
        ("SpeedMacro feature ids should be unique."));
}
}  // namespace routing
