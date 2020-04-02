#pragma once

#include "indexer/feature.hpp"

#include "geometry/rect2d.hpp"

#include <map>
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
  RoadShield(RoadShieldType const & type, std::string const & name,
             std::string const & additionalText)
  : m_type(type), m_name(name), m_additionalText(additionalText)
  {}

  inline bool operator<(RoadShield const & other) const
  {
    if (m_additionalText == other.m_additionalText)
    {
      if (m_type == other.m_type)
        return m_name < other.m_name;
      return m_type < other.m_type;
    }
    return m_additionalText < other.m_additionalText;
  }
};

// Use specific country road shield styles based on mwm feature belongs to.
std::set<RoadShield> GetRoadShields(FeatureType & f);

// Simple parsing without specific country styles.
std::set<RoadShield> GetRoadShields(std::string const & rawRoadNumber);

std::string DebugPrint(RoadShieldType shieldType);
std::string DebugPrint(RoadShield const & shield);
}  // namespace ftypes

using GeneratedRoadShields = std::map<ftypes::RoadShield, std::vector<m2::RectD>>;

