#pragma once

#include "platform/location.hpp"

#include "geometry/point2d.hpp"

#include "std/vector.hpp"

class GpsTrackFilter
{
public:
  /// Store setting for  minimal horizontal accuracy
  static void StoreMinHorizontalAccuracy(double value);

  GpsTrackFilter();

  void Process(vector<location::GpsInfo> const & inPoints,
               vector<location::GpsTrackInfo> & outPoints);

private:
  double m_minAccuracy;
  m2::PointD m_lastPt;
  bool m_hasLast;
};

