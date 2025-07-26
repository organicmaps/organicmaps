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
  RoadShield(RoadShieldType const & type, std::string_view name) : m_type(type), m_name(name) {}
  RoadShield(RoadShieldType const & type, std::string const & name, std::string const & additionalText)
    : m_type(type)
    , m_name(name)
    , m_additionalText(additionalText)
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

  inline bool operator==(RoadShield const & other) const
  {
    return (m_type == other.m_type && m_name == other.m_name && m_additionalText == other.m_additionalText);
  }
};

// Use specific country road shield styles based on mwm feature belongs to.
using RoadShieldsSetT = buffer_vector<RoadShield, 2>;
RoadShieldsSetT GetRoadShields(FeatureType & f);
RoadShieldsSetT GetRoadShields(std::string const & mwmName, std::string const & roadNumber);

// Simple parsing without specific country styles.
RoadShieldsSetT GetRoadShields(std::string const & rawRoadNumber);

// Returns names of road shields if |ft| is a "highway" feature.
std::vector<std::string> GetRoadShieldsNames(FeatureType & ft);

std::string DebugPrint(RoadShieldType shieldType);
std::string DebugPrint(RoadShield const & shield);
}  // namespace ftypes

using GeneratedRoadShields = std::map<ftypes::RoadShield, std::vector<m2::RectD>>;
