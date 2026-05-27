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

private:
  m2::PointD m_previousPoint;
  geometry::Altitude m_previousAltitude = geometry::kInvalidAltitude;
  double m_previousTimestamp = -1;
};
