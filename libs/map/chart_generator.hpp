#pragma once

#include "indexer/map_style.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <vector>

class ElevationInfo;

class ChartGenerator
{
  static double constexpr kDistanceEps = 1e-6;

public:
  static uint32_t constexpr kBPP = 4;

  explicit ChartGenerator(ElevationInfo const & info) : m_info(info) {}

  /// Generates altitude chart image (RGBA 8888) directly from ElevationInfo.
  bool Generate(uint32_t width, uint32_t height, MapStyle mapStyle, std::vector<uint8_t> & frameBuffer) const;
  /// Uses current map style from StyleReader.
  bool Generate(uint32_t width, uint32_t height, std::vector<uint8_t> & frameBuffer) const;

  /// Generates chart image from pre-built 2D geometry points.
  static void GenerateByPoints(uint32_t width, uint32_t height, std::vector<m2::PointD> const & geometry,
                               MapStyle mapStyle, std::vector<uint8_t> & frameBuffer);

  /// Converts altitude data to Y-axis pixel coordinates, preventing large waves for small deviations.
  static bool GenerateYAxisChartData(uint32_t height, double minMetersPerPxl, std::vector<double> const & altitudeDataM,
                                     std::vector<double> & yAxisDataPxl);

  static void ScaleChartData(std::vector<double> & chartData, double scale);
  static void ShiftChartData(std::vector<double> & chartData, double shift);
  static void ReflectChartData(std::vector<double> & chartData);

  /// Normalizes altitude data via sequential scan over ElevationInfo lines.
  void NormalizeAltitudes(size_t resultPointCount, std::vector<double> & uniformAltitudeDataM) const;

private:
  ElevationInfo const & m_info;
};
