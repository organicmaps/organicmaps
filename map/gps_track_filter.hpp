#pragma once

#include "platform/location.hpp"

#include "geometry/latlon.hpp"

#include "std/vector.hpp"

class IGpsTrackFilter
{
public:
  virtual ~IGpsTrackFilter() = default;

  virtual void Process(vector<location::GpsInfo> const & inPoints,
                       vector<location::GpsTrackInfo> & outPoints) = 0;
};

class GpsTrackNullFilter : public IGpsTrackFilter
{
public:
  // IGpsTrackFilter overrides
  void Process(vector<location::GpsInfo> const & inPoints,
               vector<location::GpsTrackInfo> & outPoints) override;
};

class GpsTrackFilter : public IGpsTrackFilter
{
public:
  /// Store setting for  minimal horizontal accuracy
  static void StoreMinHorizontalAccuracy(double value);

  GpsTrackFilter();

  // IGpsTrackFilter overrides
  void Process(vector<location::GpsInfo> const & inPoints,
               vector<location::GpsTrackInfo> & outPoints) override;

private:
  double m_minAccuracy;
  location::GpsInfo m_lastInfo;
  bool m_hasLastInfo;
};

