#pragma once

#include "search/reverse_geocoder.hpp"

// Note: this class is NOT thread-safe.
class CachingAddressGetter
{
public:
  search::ReverseGeocoder::Address GetAddressAtPoint(DataSource const & dataSource,
                                                     m2::PointD const & pt) const
  {
    if (pt == m_cache.m_point)
      return m_cache.m_address;

    double const kDistanceThresholdMeters = 0.5;
    m_cache.m_point = pt;

    search::ReverseGeocoder const coder(dataSource);
    search::ReverseGeocoder::Address address;
    coder.GetNearbyAddress(pt, m_cache.m_address);

    // We do not init nearby address info for points that are located
    // outside of the nearby building.
    if (address.GetDistance() >= kDistanceThresholdMeters)
      m_cache.m_address = {};

    return m_cache.m_address;
  }
private:
  struct Cache
  {
    m2::PointD m_point;
    search::ReverseGeocoder::Address m_address;
  };

  mutable Cache m_cache;
};
