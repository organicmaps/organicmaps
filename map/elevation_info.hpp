#pragma once

#include "map/track.hpp"

#include "geometry/point_with_altitude.hpp"

#include <cstdint>
#include <string>
#include <vector>

class ElevationInfo
{
public:
  struct Point
  {
    Point(double distance, geometry::Altitude altitude)
      : m_distance(distance), m_altitude(altitude)
    {
    }

    double m_distance;
    geometry::Altitude m_altitude;
  };

  using Points = std::vector<Point>;

  enum Difficulty : uint8_t
  {
    Unknown,
    Easy,
    Medium,
    Hard
  };

  ElevationInfo() = default;
  explicit ElevationInfo(Track const & track);

  kml::TrackId GetId() const { return m_id; };
  std::string const & GetName() const { return m_name; }
  size_t GetSize() const { return m_points.size(); };
  Points const & GetPoints() const { return m_points; };
  uint16_t GetAscent() const { return m_ascent; }
  uint16_t GetDescent() const { return m_descent; }
  uint16_t GetMinAltitude() const { return m_minAltitude; }
  uint16_t GetMaxAltitude() const { return m_maxAltitude; }
  uint8_t GetDifficulty() const { return m_difficulty; }
  uint32_t GetDuration() const { return m_duration; }

private:
  kml::TrackId m_id = kml::kInvalidTrackId;
  std::string m_name;
  // Points with distance from start of the track and altitude.
  Points m_points;
  // Ascent in meters.
  uint16_t m_ascent = 0;
  // Descent in meters.
  uint16_t m_descent = 0;
  // Altitude in meters.
  uint16_t m_minAltitude = 0;
  // Altitude in meters.
  uint16_t m_maxAltitude = 0;
  // Some digital difficulty level with value in range [0-kMaxDifficulty]
  // or kInvalidDifficulty when difficulty is not found or incorrect.
  Difficulty m_difficulty = Difficulty::Unknown;
  // Duration in seconds.
  uint32_t m_duration = 0;
};
