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
  {
    LOG(LERROR, ("Property not found for key:", key));
    return;
  }

  if (!strings::to_any(it->second, value))
    LOG(LERROR, ("Conversion is not possible for key", key, "string representation is", it->second));

}

template <>
void FillProperty<ElevationInfo::Difficulty>(kml::Properties const & properties, std::string const & key, ElevationInfo::Difficulty & value)
{
  auto const it = properties.find(key);
  if (it == properties.cend())
  {
    LOG(LERROR, ("Property not found for key:", key));
    return;
  }

  uint8_t difficulty;
  if (!strings::to_any(it->second, difficulty))
  {
    LOG(LERROR, ("Conversion is not possible for key", key, "string representation is", it->second));
    return;
  }
  if (difficulty > kMaxDifficulty)
  {
    LOG(LERROR, ("Invalid difficulty value", difficulty));
    return;
  }
  value = static_cast<ElevationInfo::Difficulty>(difficulty);
}
}  // namespace

ElevationInfo::ElevationInfo(Track const & track)
  : m_id(track.GetId())
  , m_name(track.GetName())
{
  // (Distance, Elevation) chart doesn't have a sence for multiple track's geometry.
  auto const & points = track.GetSingleGeometry();
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

  FillProperty(properties, kAscentKey, m_ascent);
  FillProperty(properties, kDescentKey, m_descent);
  FillProperty(properties, kLowestPointKey, m_minAltitude);
  FillProperty(properties, kHighestPointKey, m_maxAltitude);
  FillProperty(properties, kDifficultyKey, m_difficulty);
  FillProperty(properties, kDurationKey, m_duration);
}
