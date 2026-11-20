#pragma once

#include "platform/location.hpp"

#include "kml/types.hpp"

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

  /// Incrementally adds a GPS point (for live track recording).
  void AddGpsInfoPoint(location::GpsInfo const & point);

  /// Show elevation once the recording has positive distance, complete heights and a nonzero height.
  /// One accepted fix without height hides the chart and elevation totals for the rest of the recording.
  bool HasGpsElevationProfile() const
  {
    return m_length > 0 && !m_hasMissingAltitude && (m_minElevation != 0 || m_maxElevation != 0);
  }

private:
  m2::PointD m_previousPoint;
  geometry::Altitude m_previousAltitude = geometry::kInvalidAltitude;
  bool m_hasMissingAltitude = false;
  double m_previousTimestamp = -1;
};
