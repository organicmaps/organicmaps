#pragma once

#include "search/reverse_geocoder.hpp"

// TODO: get rid of this class when version 8.6 will not be supported.
// TODO: because of fast algorithm and new mwm section which were implemented for 9.0.
// Note: this class is NOT thread-safe.
class CachingAddressGetter
{
public:
  search::ReverseGeocoder::Address GetAddressAtPoint(DataSource const & dataSource,
                                                     m2::PointD const & pt) const
  {
    if (pt.EqualDxDy(m_cache.m_point, kMwmPointAccuracy))
      return m_cache.m_address;

    double const kDistanceThresholdMeters = 0.5;
    m_cache.m_point = pt;
    m_cache.m_address = {};

    search::ReverseGeocoder const coder(dataSource);
    coder.GetNearbyAddress(pt, kDistanceThresholdMeters, m_cache.m_address);

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
