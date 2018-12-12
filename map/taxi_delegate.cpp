#include "map/taxi_delegate.hpp"

#include "search/city_finder.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/index.hpp"
#include "storage/storage.hpp"

#include "coding/string_utf8_multilang.hpp"

TaxiDelegate::TaxiDelegate(storage::Storage const & st, storage::CountryInfoGetter const & ig,
                           search::CityFinder & cf)
  : m_storage(st), m_infoGetter(ig), m_cityFinder(cf)
{
}

storage::TCountriesVec TaxiDelegate::GetCountryIds(m2::PointD const & point)
{
  auto const countryId = m_infoGetter.GetRegionCountryId(point);
  storage::TCountriesVec topmostCountryIds;
  m_storage.GetTopmostNodesFor(countryId, topmostCountryIds);
  return topmostCountryIds;
}

std::string TaxiDelegate::GetCityName(m2::PointD const & point)
{
  return m_cityFinder.GetCityName(point, StringUtf8Multilang::kEnglishCode);
}

storage::TCountryId TaxiDelegate::GetMwmId(m2::PointD const & point)
{
  return m_infoGetter.GetRegionCountryId(point);
}
