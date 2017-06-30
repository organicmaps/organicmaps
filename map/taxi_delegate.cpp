#include "map/taxi_delegate.hpp"

#include "search/city_finder.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/index.hpp"
#include "storage/storage.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "geometry/mercator.hpp"

TaxiDelegate::TaxiDelegate(storage::Storage const & st, storage::CountryInfoGetter const & ig,
                           search::CityFinder & cf)
  : m_storage(st), m_infoGetter(ig), m_cityFinder(cf)
{
}

storage::TCountriesVec TaxiDelegate::GetCountryIds(ms::LatLon const & latlon)
{
  m2::PointD const point = MercatorBounds::FromLatLon(latlon);
  auto const countryId = m_infoGetter.GetRegionCountryId(point);
  storage::TCountriesVec topmostCountryIds;
  m_storage.GetTopmostNodesFor(countryId, topmostCountryIds);
  return topmostCountryIds;
}

std::string TaxiDelegate::GetCityName(ms::LatLon const & latlon)
{
  m2::PointD const point = MercatorBounds::FromLatLon(latlon);
  return m_cityFinder.GetCityName(point, StringUtf8Multilang::kEnglishCode);
}
