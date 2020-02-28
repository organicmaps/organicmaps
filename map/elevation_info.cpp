#include "map/elevation_info.hpp"

#include "base/string_utils.hpp"

namespace
{
std::string const kAscentKey = "ascent";
std::string const kDescentKey = "descent";
std::string const kLowestPointKey = "lowest_point";
std::string const kHighestPointKey = "highest_point";
std::string const kDifficultyKey = "difficulty";
std::string const kDurationKey = "duration";

template <typename T>
void FillUintProperty(kml::Properties const & properties, std::string const & key, T & value)
{
  auto const it = properties.find(kAscentKey);
  if (it != properties.cend() && strings::to_any(it->second, value))
    LOG(LERROR, ("Conversion is not possible for key", key, "string representation is", it->second));
}
}  // namespace

ElevationInfo::ElevationInfo(Track const & track)
  : m_id(track.GetId())
  , m_name(track.GetName())
{
  auto const & points = track.GetPointsWithAltitudes();

  if (points.empty())
    return;

  m_points.reserve(points.size());
  m_points.emplace_back(0, points[0].GetAltitude());
  double distance = 0.0;
  for (size_t i = 1; i < points.size(); ++i)
  {
    distance += mercator::DistanceOnEarth(points[i - 1].GetPoint(), points[i].GetPoint());
    m_points.emplace_back(distance, points[i].GetAltitude());
  }

  auto const & properties = track.GetData().m_properties;

  FillUintProperty(properties, kAscentKey, m_ascent);
  FillUintProperty(properties, kDescentKey, m_descent);
  FillUintProperty(properties, kLowestPointKey, m_minAltitude);
  FillUintProperty(properties, kHighestPointKey, m_maxAltitude);
  FillUintProperty(properties, kDifficultyKey, m_difficulty);
  FillUintProperty(properties, kDurationKey, m_duration);
}
