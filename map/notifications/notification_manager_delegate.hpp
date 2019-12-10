#pragma once

#include "map/notifications/notification_manager.hpp"

#include "storage/storage_defines.hpp"

#include "geometry/point2d.hpp"

#include <string>

class DataSource;
class CachingAddressGetter;

namespace search
{
class CityFinder;
}

namespace ugc
{
class Api;
}

namespace storage
{
class Storage;
class CountryInfoGetter;
}

namespace notifications
{
class NotificationManagerDelegate : public NotificationManager::Delegate
{
public:
  NotificationManagerDelegate(DataSource const & dataSource, search::CityFinder & cityFinder,
                              CachingAddressGetter & addressGetter, ugc::Api & ugcApi,
                              storage::Storage & storage,
                              storage::CountryInfoGetter & countryInfoGetter);

  // NotificationManager::Delegate overrides:
  ugc::Api & GetUGCApi() override;
  std::unordered_set<storage::CountryId> GetDescendantCountries(
      storage::CountryId const & country) const override;
  storage::CountryId GetCountryAtPoint(m2::PointD const & pt) const override;
  std::string GetAddress(m2::PointD const & pt) override;

private:
  DataSource const & m_dataSource;
  search::CityFinder & m_cityFinder;
  CachingAddressGetter & m_addressGetter;
  ugc::Api & m_ugcApi;
  storage::Storage & m_storage;
  storage::CountryInfoGetter & m_countryInfoGetter;
};
}  // namespace notifications
