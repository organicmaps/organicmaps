#include "notification_manager_delegate.hpp"

#include "ugc/api.hpp"

#include "map/utils.hpp"

#include "search/city_finder.hpp"

#include "indexer/feature_decl.hpp"

#include "platform/preferred_languages.hpp"

#include "coding/string_utf8_multilang.hpp"

namespace notifications
{
NotificationManagerDelegate::NotificationManagerDelegate(DataSource const & dataSource,
                                                         search::CityFinder & cityFinder,
                                                         ugc::Api & ugcApi)
  : m_dataSource(dataSource), m_cityFinder(cityFinder), m_ugcApi(ugcApi)
{
}

ugc::Api & NotificationManagerDelegate::GetUGCApi()
{
  return m_ugcApi;
}

string NotificationManagerDelegate::GetAddress(m2::PointD const & pt)
{
  auto const address = utils::GetAddressAtPoint(m_dataSource, pt).FormatAddress();
  auto const langIndex = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
  auto const city = m_cityFinder.GetCityName(pt, langIndex);

  if (address.empty())
    return city;

  if (city.empty())
    return address;

  return city + " ," + address;
}
}  // namespace notifications
