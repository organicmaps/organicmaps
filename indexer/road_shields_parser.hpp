#pragma once

#include "indexer/feature.hpp"

#include <string>
#include <vector>

namespace ftypes
{
enum class RoadShieldType
{
  Default = 0,
  Generic_White,  // The same as default, for semantics
  Generic_Green,
  Generic_Blue,
  Generic_Red,
  Generic_Orange,
  US_Interstate,
  US_Highway,
  UK_Highway,
  Hidden,
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

  inline bool operator<(RoadShield const & other) const
  {
    return m_type < other.m_type || m_name < other.m_name || m_additionalText < other.m_additionalText;
  }
};

std::set<RoadShield> GetRoadShields(FeatureType const & f);
std::string DebugPrint(RoadShieldType shieldType);
std::string DebugPrint(RoadShield const & shield);
}  // namespace ftypes
