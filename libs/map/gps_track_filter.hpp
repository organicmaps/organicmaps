#pragma once

#include "platform/location.hpp"

#include <vector>

class IGpsTrackFilter
{
public:
  virtual ~IGpsTrackFilter() = default;

  using GpsVectorT = std::vector<location::GpsInfo>;
  virtual void Process(GpsVectorT const & inPoints, GpsVectorT & outPoints) = 0;
  // Returns the last fix if sampling deferred it.
  virtual location::GpsInfo const * GetDeferredPoint() const { return nullptr; }
  virtual void Reset() {}
};

class GpsTrackNullFilter : public IGpsTrackFilter
{
public:
  // IGpsTrackFilter overrides
  void Process(GpsVectorT const & inPoints, GpsVectorT & outPoints) override;
};

class GpsTrackFilter : public IGpsTrackFilter
{
public:
  /// Store setting for  minimal horizontal accuracy
  static void StoreMinHorizontalAccuracy(double value);

  GpsTrackFilter();

  // IGpsTrackFilter overrides
  void Process(GpsVectorT const & inPoints, GpsVectorT & outPoints) override;
  location::GpsInfo const * GetDeferredPoint() const override;
  void Reset() override;

private:
  bool IsGoodPoint(location::GpsInfo const & info) const;
  bool IsGoodVector(location::GpsInfo const & info) const;

  location::GpsInfo const & GetLastInfo() const;
  location::GpsInfo const & GetLastAcceptedInfo() const;

  void AddLastInfo(location::GpsInfo const & info);

  double m_minAccuracy;

  location::GpsInfo m_lastInfo[2];
  size_t m_countLastInfo = 0;

  location::GpsInfo m_lastAcceptedInfo;
};
