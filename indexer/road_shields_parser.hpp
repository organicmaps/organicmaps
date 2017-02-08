#pragma once

#include "indexer/feature.hpp"

#include <string>
#include <vector>

namespace ftypes
{
enum class RoadShieldType
{
  Default = 0,
  Generic_Green,
  Generic_Blue,
  Generic_Red,
  Generic_Orange,
  US_Interstate,
  US_Highway,
  UK_Highway,
  Count
};

struct RoadShield
{
  RoadShieldType m_type;
  std::string m_name;
  std::string m_additionalText;

  RoadShield() = default;
  RoadShield(RoadShieldType const & type, std::string const & name)
  : m_type(type), m_name(name)
  {}
  RoadShield(RoadShieldType const & type, std::string const & name, std::string const & additionalText)
  : m_type(type), m_name(name), m_additionalText(additionalText)
  {}
};

std::vector<RoadShield> GetRoadShields(FeatureType const & f);
}  // namespace ftypes
