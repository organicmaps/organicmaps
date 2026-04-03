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

  /// Replaces altitude of points where slope exceeds maxSlopePercent with interpolated values.
  /// @param[in] maxSlopePercent Maximum plausible slope (100 = 45°). Default 100% covers steep alpine trails.
  void SmoothSlopeOutliers(double maxSlopePercent = 100.0);

  /// @param[in] altitudeDeviation Simplification threshold in meters (~sqrt(2) by default).
  void Simplify(double altitudeDeviation = 1.415);

  using Altitude = geometry::Altitude;
  static Altitude constexpr kDefThresholdMWM = 5;
  static Altitude constexpr kDefThresholdGPS = 10;

  struct AltitudesInfo
  {
    uint32_t m_totalAscentRaw = 0;
    uint32_t m_totalDescentRaw = 0;
    uint32_t m_totalAscentFiltered = 0;
    uint32_t m_totalDescentFiltered = 0;
    Altitude m_minAltitude = std::numeric_limits<Altitude>::max();
    Altitude m_maxAltitude = std::numeric_limits<Altitude>::min();

    /// Falls back to raw values if filtered are zero (e.g. almost flat tracks).
    uint32_t GetTotalAscent() const { return m_totalAscentFiltered > 0 ? m_totalAscentFiltered : m_totalAscentRaw; }
    uint32_t GetTotalDescent() const { return m_totalDescentFiltered > 0 ? m_totalDescentFiltered : m_totalDescentRaw; }
  };

  /// Calculates all altitude statistics in one pass.
  /// @param[in] threshold Minimum altitude change for filtered ascent/descent (threshold accumulation).
  AltitudesInfo CalculateAltitudesInfo(Altitude threshold) const;

  /// Iterates all points with cumulative distances across all lines.
  /// @param[in] fn Called with (double cumulativeDistance, geometry::Altitude altitude) for each point.
  template <typename Fn>
  void ForEachPoint(Fn && fn) const
  {
    double cumulativeOffset = 0;
    for (auto const & line : m_lines)
    {
      for (auto const & point : line)
        fn(cumulativeOffset + point.m_distance, point.m_altitude);

      if (!line.empty())
        cumulativeOffset += line.back().m_distance;
    }
  }

  /// Generates altitude chart image (RGBA 8888).
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
