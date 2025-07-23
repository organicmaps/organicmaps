#pragma once

#include "platform/location.hpp"

#include <vector>

class IGpsTrackFilter
{
public:
  virtual ~IGpsTrackFilter() = default;

  using GpsVectorT = std::vector<location::GpsInfo>;
  virtual void Process(GpsVectorT const & inPoints, GpsVectorT & outPoints) = 0;
  virtual void Finalize(GpsVectorT & outPoints) {}
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
  void Finalize(GpsVectorT & outPoints) override;

private:
  bool IsGoodPoint(location::GpsInfo const & info) const;
  bool IsGoodVector(location::GpsInfo const & info) const;

  location::GpsInfo const & GetLastInfo() const;
  location::GpsInfo const & GetLastAcceptedInfo() const;

  void AddLastInfo(location::GpsInfo const & info);
  void AddLastAcceptedInfo(location::GpsInfo const & info);

  double m_minAccuracy;

  location::GpsInfo m_lastInfo[2];
  size_t m_countLastInfo;

  location::GpsInfo m_lastAcceptedInfo[2];
  size_t m_countAcceptedInfo;
};
