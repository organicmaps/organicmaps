#include "map/elevation_info.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

namespace
{
static uint8_t constexpr kMaxDifficulty = ElevationInfo::Difficulty::Hard;

std::string const kAscentKey = "ascent";
std::string const kDescentKey = "descent";
std::string const kLowestPointKey = "lowest_point";
std::string const kHighestPointKey = "highest_point";
std::string const kDifficultyKey = "difficulty";
std::string const kDurationKey = "duration";

template <typename T>
void FillProperty(kml::Properties const & properties, std::string const & key, T & value)
{
  auto const it = properties.find(key);
  if (it == properties.cend())
    LOG(LERROR, ("Property not found for key:", key));
  else
  {
    if (!strings::to_any(it->second, value))
      LOG(LERROR, ("Conversion is not possible for key", key, "string representation is", it->second));
  }
}
}  // namespace

ElevationInfo::ElevationInfo(Track const & track)
  : m_id(track.GetId())
  , m_name(track.GetName())
{
  // (Distance, Elevation) chart doesn't have a sence for multiple track's geometry.
  auto const & trackData = track.GetData();
  ASSERT_EQUAL(trackData.m_geometry.m_lines.size(), 1, ());
  auto const & points = track.GetSingleGeometry();
  if (points.empty())
    return;

  m_points.reserve(points.size());

  auto const & baseAltitude = points[0].GetAltitude();
  m_points.emplace_back(0, baseAltitude);
  m_minAltitude = baseAltitude;
  m_maxAltitude = baseAltitude;

  double distance = 0.0;
  for (size_t i = 1; i < points.size(); ++i)
  {
    distance += mercator::DistanceOnEarth(points[i - 1].GetPoint(), points[i].GetPoint());
    m_points.emplace_back(distance, points[i].GetAltitude());

    auto const & previousPointAltitude = points[i - 1].GetAltitude();
    auto const & currentPointAltitude = points[i].GetAltitude();
    auto const deltaAltitude = currentPointAltitude - previousPointAltitude;
    if (deltaAltitude > 0)
      m_ascent += deltaAltitude;
    else
      m_descent -= deltaAltitude;

    if (currentPointAltitude < m_minAltitude)
      m_minAltitude = currentPointAltitude;
    if (currentPointAltitude > m_maxAltitude)
      m_maxAltitude = currentPointAltitude;
  }

  auto const & timestamps = trackData.m_geometry.m_timestamps[0];
  m_duration = timestamps.back() - timestamps.front();
  ASSERT_GREATER(m_duration, 0, ("Track duration is less than zero", GetId()));
  
  m_difficulty = Difficulty::Unknown;
}
