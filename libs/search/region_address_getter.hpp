#pragma once

#include "search/city_finder.hpp"
#include "search/region_info_getter.hpp"
#include "search/reverse_geocoder.hpp"

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

namespace search
{
// Note: This class is NOT thread-safe.
class RegionAddressGetter
{
public:
  RegionAddressGetter(DataSource const & dataSource, storage::CountryInfoGetter const & infoGetter);

  ReverseGeocoder::RegionAddress GetNearbyRegionAddress(m2::PointD const & center);
  std::string GetLocalizedRegionAddress(ReverseGeocoder::RegionAddress const & addr) const;
  std::string GetLocalizedRegionAddress(m2::PointD const & center);

private:
  ReverseGeocoder m_reverseGeocoder;
  RegionInfoGetter m_nameGetter;
  CityFinder m_cityFinder;

  storage::CountryInfoGetter const & m_infoGetter;
};
}  // namespace search
