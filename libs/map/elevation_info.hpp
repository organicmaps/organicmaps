#pragma once

#include "kml/types.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point_with_altitude.hpp"

#include "platform/location.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct ElevationInfo
{
public:
  struct Point
  {
    Point(geometry::PointWithAltitude point, double distance) : m_point(point), m_distance(distance) {}
    geometry::PointWithAltitude const m_point;
    double const m_distance;
  };

  using Points = std::vector<Point>;
  using GpsPoints = std::vector<location::GpsInfo>;
  using GeometryLine = kml::MultiGeometry::LineT;
  using SegmentsDistances = std::vector<double>;

  enum Difficulty : uint8_t
  {
    Unknown,
    Easy,
    Medium,
    Hard
  };

  ElevationInfo() = default;
  explicit ElevationInfo(std::vector<GeometryLine> const & lines);

  void AddGpsPoints(GpsPoints const & points);

  size_t GetSize() const { return m_points.size(); }
  Points const & GetPoints() const { return m_points; }
  uint8_t GetDifficulty() const { return m_difficulty; }
  SegmentsDistances const & GetSegmentsDistances() const { return m_segmentsDistances; }

private:
  // Points with distance from start of the track and altitude.
  Points m_points;
  // Some digital difficulty level with value in range [0-kMaxDifficulty]
  // or kInvalidDifficulty when difficulty is not found or incorrect.
  Difficulty m_difficulty = Difficulty::Unknown;
  // Distances to the start of each segment.
  SegmentsDistances m_segmentsDistances;

  void AddPoints(GeometryLine const & line, bool isNewSegment = false);
};
