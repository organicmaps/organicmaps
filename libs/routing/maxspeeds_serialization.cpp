#include "routing/maxspeeds_serialization.hpp"

namespace routing
{

MaxspeedsSerializer::FeatureSpeedMacro::FeatureSpeedMacro(uint32_t featureID, Maxspeed const & speed,
                                                          MaxspeedConverter const & converter)
  : m_featureID(featureID)
  , m_forward(converter.SpeedToMacro({speed.GetForward(), speed.GetUnits()}))
  , m_backward(converter.SpeedToMacro({speed.GetBackward(), speed.GetUnits()}))
{
  // We store bidirectional speeds only if they are not equal, see Maxspeed::IsBidirectional() comments.
  if (m_forward == m_backward)
    m_backward = SpeedMacro::Undefined;

  if (speed.HasConditional())
  {
    m_conditional = converter.SpeedToMacro({speed.GetConditional(), speed.GetUnits()});
    m_conditionalTime = speed.GetConditionalTime();
  }
}

std::vector<uint8_t> MaxspeedsSerializer::GetForwardMaxspeeds(std::vector<FeatureSpeedMacro> const & speeds,
                                                              uint32_t & maxFeatureID)
{
  std::vector<uint8_t> result;
  for (auto const & s : speeds)
  {
    if (!s.IsComplex())
    {
      result.push_back(static_cast<uint8_t>(s.m_forward));
      maxFeatureID = s.m_featureID;
    }
  }
  return result;
}
}  // namespace routing
