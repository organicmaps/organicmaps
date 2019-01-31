#pragma once

#include "map/notifications/notification_manager.hpp"

#include "geometry/point2d.hpp"

class DataSource;

namespace search
{
class CityFinder;
}

namespace ugc
{
class Api;
}

namespace notifications
{
class NotificationManagerDelegate : public NotificationManager::Delegate
{
public:
  NotificationManagerDelegate(DataSource const & dataSource, search::CityFinder & cityFinder,
                              ugc::Api & ugcApi);

  // NotificationManager::Delegate overrides:
  ugc::Api & GetUGCApi() override;
  string GetAddress(m2::PointD const & pt) override;

private:
  DataSource const & m_dataSource;
  search::CityFinder & m_cityFinder;
  ugc::Api & m_ugcApi;
};
}
