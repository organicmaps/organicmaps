#include "map/notifications/notification_manager_delegate.hpp"

#include "map/caching_address_getter.hpp"

#include "ugc/api.hpp"

#include "search/city_finder.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "coding/string_utf8_multilang.hpp"

namespace notifications
{
NotificationManagerDelegate::NotificationManagerDelegate(DataSource const & dataSource,
                                                         search::CityFinder & cityFinder,
                                                         CachingAddressGetter & addressGetter,
                                                         ugc::Api & ugcApi,
                                                         storage::Storage & storage,
                                                         storage::CountryInfoGetter & countryInfoGetter)
  : m_dataSource(dataSource)
  , m_cityFinder(cityFinder)
  , m_addressGetter(addressGetter)
  , m_ugcApi(ugcApi)
  , m_storage(storage)
  , m_countryInfoGetter(countryInfoGetter)
{
}

ugc::Api & NotificationManagerDelegate::GetUGCApi()
{
  return m_ugcApi;
}

std::unordered_set<storage::CountryId> NotificationManagerDelegate::GetDescendantCountries(
    storage::CountryId const & country) const
{
  std::unordered_set<storage::CountryId> result;
  auto const fn = [&result](storage::CountryId const & countryId, bool isGroupNode)
  {
    if (isGroupNode)
      return;
    result.insert(countryId);
  };
  m_storage.ForEachInSubtree(country, fn);

  return result;
}

storage::CountryId NotificationManagerDelegate::GetCountryAtPoint(m2::PointD const & pt) const
{
  return m_countryInfoGetter.GetRegionCountryId(pt);
}

std::string NotificationManagerDelegate::GetAddress(m2::PointD const & pt)
{
  auto const address = m_addressGetter.GetAddressAtPoint(m_dataSource, pt).FormatAddress();
  auto const city = m_cityFinder.GetCityReadableName(pt);

  if (address.empty())
    return city;

  if (city.empty())
    return address;

  return address + ", " + city;
}
}  // namespace notifications
