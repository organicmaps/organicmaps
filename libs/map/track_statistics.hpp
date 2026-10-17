#pragma once

#include "kml/types.hpp"

class ElevationInfo;

struct TrackStatistics
{
  double m_length = 0;
  double m_duration = 0;
  double m_ascent = 0;
  double m_descent = 0;
  geometry::Altitude m_minElevation = geometry::kDefaultAltitudeMeters;
  geometry::Altitude m_maxElevation = geometry::kDefaultAltitudeMeters;

  std::string GetFormattedLength() const;
  std::string GetFormattedDuration() const;
  std::string GetFormattedAscent() const;
  std::string GetFormattedDescent() const;
  std::string GetFormattedMinElevation() const;
  std::string GetFormattedMaxElevation() const;

  /// Calculates duration from timestamps in geometry.
  void CalculateDuration(kml::MultiGeometry const & geometry);

  /// Calculates altitude statistics from the processed elevation profile.
  void CalculateAltitudes(ElevationInfo const & elevation, geometry::Altitude threshold);
};
