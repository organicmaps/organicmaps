#include "notification_manager_delegate.hpp"

#include "ugc/api.hpp"

#include "map/caching_address_getter.hpp"

#include "search/city_finder.hpp"

#include "indexer/feature_decl.hpp"

#include "coding/string_utf8_multilang.hpp"

namespace notifications
{
NotificationManagerDelegate::NotificationManagerDelegate(DataSource const & dataSource,
                                                         search::CityFinder & cityFinder,
                                                         CachingAddressGetter & addressGetter,
                                                         ugc::Api & ugcApi)
  : m_dataSource(dataSource)
  , m_cityFinder(cityFinder)
  , m_addressGetter(addressGetter)
  , m_ugcApi(ugcApi)
{
}

ugc::Api & NotificationManagerDelegate::GetUGCApi()
{
  return m_ugcApi;
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
