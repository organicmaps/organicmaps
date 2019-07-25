#include "search/region_address_getter.hpp"

#include "search/city_finder.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/preferred_languages.hpp"

namespace search
{
RegionAddressGetter::RegionAddressGetter(DataSource const & dataSource, storage::CountryInfoGetter const & infoGetter,
                                         CityFinder & cityFinder)
  : m_reverseGeocoder(dataSource)
  , m_infoGetter(infoGetter)
  , m_cityFinder(cityFinder)
{
  m_nameGetter.LoadCountriesTree();
  m_nameGetter.SetLocale(languages::GetCurrentNorm());
}

ReverseGeocoder::RegionAddress RegionAddressGetter::GetNearbyRegionAddress(m2::PointD const & center)
{
  return ReverseGeocoder::GetNearbyRegionAddress(center, m_infoGetter, m_cityFinder);
}

std::string RegionAddressGetter::GetLocalizedRegionAdress(ReverseGeocoder::RegionAddress const & addr) const
{
  return m_reverseGeocoder.GetLocalizedRegionAdress(addr, m_nameGetter);
}

std::string RegionAddressGetter::GetLocalizedRegionAdress(m2::PointD const & center)
{
  auto const addr = ReverseGeocoder::GetNearbyRegionAddress(center, m_infoGetter, m_cityFinder);
  return m_reverseGeocoder.GetLocalizedRegionAdress(addr, m_nameGetter);
}
}  // namespace search
