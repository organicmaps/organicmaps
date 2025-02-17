#pragma once

#include "platform/location.hpp"

#include "kml/types.hpp"

struct TrackStatistics
{
  TrackStatistics();
  explicit TrackStatistics(kml::MultiGeometry const & geometry);

  double m_length;
  double m_duration;
  double m_ascent;
  double m_descent;
  int16_t m_minElevation;
  int16_t m_maxElevation;

  void AddGpsInfoPoint(location::GpsInfo const & point);
private:
  void AddPoints(kml::MultiGeometry::LineT const & line, bool isNewSegment);
  void AddTimestamps(kml::MultiGeometry::TimeT const & timestamps, bool isNewSegment);
  void InitializeNewSegment(geometry::PointWithAltitude const & firstPoint);
  void ProcessPoints(kml::MultiGeometry::LineT const & points, size_t startIndex);
  bool HasNoPoints() const;

  geometry::PointWithAltitude m_previousPoint;
  double m_previousTimestamp;
};
