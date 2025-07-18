#pragma once

#include "platform/location.hpp"

#include "kml/types.hpp"

struct TrackStatistics
{
  using Points = kml::MultiGeometry::LineT;
  using Timestamps = kml::MultiGeometry::TimeT;

  TrackStatistics();
  explicit TrackStatistics(kml::MultiGeometry const & geometry);

  double m_length;
  double m_duration;
  double m_ascent;
  double m_descent;
  geometry::Altitude m_minElevation;
  geometry::Altitude m_maxElevation;

  std::string GetFormattedLength() const;
  std::string GetFormattedDuration() const;
  std::string GetFormattedAscent() const;
  std::string GetFormattedDescent() const;
  std::string GetFormattedMinElevation() const;
  std::string GetFormattedMaxElevation() const;

  void AddGpsInfoPoint(location::GpsInfo const & point);
private:
  void AddPoints(Points const & points);
  void AddTimestamps(Timestamps const & timestamps);
  bool HasNoPoints() const;

  geometry::PointWithAltitude m_previousPoint;
  double m_previousTimestamp;
};
