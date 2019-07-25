#pragma once

#include "search/region_info_getter.hpp"
#include "search/reverse_geocoder.hpp"

namespace storage
{
class CountryInfoGetter;
}  // storage

namespace search
{
class CityFinder;

class RegionAddressGetter
{
public:
  RegionAddressGetter(DataSource const & dataSource, storage::CountryInfoGetter const & infoGetter,
                      CityFinder & cityFinder);

  ReverseGeocoder::RegionAddress GetNearbyRegionAddress(m2::PointD const & center);
  std::string GetLocalizedRegionAdress(ReverseGeocoder::RegionAddress const & addr) const;
  std::string GetLocalizedRegionAdress(m2::PointD const & center);

private:
  ReverseGeocoder m_reverseGeocoder;
  RegionInfoGetter m_nameGetter;

  storage::CountryInfoGetter const & m_infoGetter;
  CityFinder & m_cityFinder;
};
}  // namespace search
