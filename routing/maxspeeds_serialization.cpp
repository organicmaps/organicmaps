#include "routing/maxspeeds_serialization.hpp"

namespace routing
{
std::vector<uint8_t> MaxspeedsSerializer::GetForwardMaxspeeds(std::vector<FeatureSpeedMacro> const & speeds,
                                                              uint32_t & maxFeatureID)
{
  std::vector<uint8_t> result;
  for (auto const & s : speeds)
  {
    if (!s.IsBidirectional())
    {
      result.push_back(static_cast<uint8_t>(s.m_forward));
      maxFeatureID = s.m_featureID;
    }
  }
  return result;
}
}  // namespace routing
