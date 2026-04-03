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

  /// Assigns from route segment data (single line).
  /// @param[in] segDistances N-1 cumulative distances between consecutive points.
  /// @param[in] altitudes N altitude values (one per point).
  void Assign(std::vector<double> const & segDistances, geometry::Altitudes const & altitudes);

  /// Each inner vector corresponds to a geometry line.
  /// Distances are independent per line (each starts from 0).
  std::vector<Points> const & GetLines() const { return m_lines; }
  uint8_t GetDifficulty() const { return m_difficulty; }
  bool IsEmpty() const { return m_lines.empty(); }

  /// @param[in] altitudeDeviation Simplification threshold in meters (~sqrt(2) by default).
  void Simplify(double altitudeDeviation = 1.415);

  /// Threshold accumulation: only counts altitude change when accumulated delta exceeds threshold.
  static geometry::Altitude constexpr kDefThresholdMWM = 5;
  static geometry::Altitude constexpr kDefThresholdGPS = 10;
  void CalculateAscentDescent(uint32_t & totalAscentM, uint32_t & totalDescentM, geometry::Altitude threshold) const;

  /// Generates altitude chart image (RGBA 8888). Flattens lines to cumulative distances.
  bool GenerateRouteAltitudeChart(uint32_t width, uint32_t height, std::vector<uint8_t> & imageRGBAData) const;

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
