#include "routing/maxspeeds_serialization.hpp"

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
}  // namespace routing
