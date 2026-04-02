#pragma once

#include "kml/types.hpp"

#include "geometry/point_with_altitude.hpp"

#include "platform/location.hpp"

#include <vector>

struct ElevationInfo
{
public:
  struct Point
  {
    double m_distance;
    geometry::Altitude m_altitude;
  };

  using Points = std::vector<Point>;
  using GeometryLine = kml::MultiGeometry::LineT;

  enum Difficulty : uint8_t
  {
    Unknown,
    Easy,
    Medium,
    Hard
  };

  ElevationInfo() = default;
  explicit ElevationInfo(std::vector<GeometryLine> const & lines);

  /// Each inner vector corresponds to a geometry line.
  /// Distances are independent per line (each starts from 0).
  std::vector<Points> const & GetLines() const { return m_lines; }
  uint8_t GetDifficulty() const { return m_difficulty; }
  bool IsEmpty() const { return m_lines.empty(); }

protected:
  std::vector<Points> m_lines;
  Difficulty m_difficulty = Difficulty::Unknown;
};

/// Extends ElevationInfo with incremental GPS point addition for track recording.
class GpsTrackElevation : public ElevationInfo
{
public:
  using GpsPoints = std::vector<location::GpsInfo>;

  GpsTrackElevation() = default;

  void AddGpsPoints(GpsPoints const & points);
  void Clear();

  size_t GetSize() const;

private:
  ms::LatLon m_lastLatLon;
  double m_lastDistance = 0;
  bool m_hasLastPoint = false;
};
