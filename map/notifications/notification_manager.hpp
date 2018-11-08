#pragma once

#include "map/notifications/notification_queue.hpp"

#include "metrics/eye.hpp"

#include <string>

#include <boost/optional.hpp>

namespace notifications
{
class NotificationManager : public eye::Subscriber
{
public:
  friend class NotificationManagerForTesting;

  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual storage::TCountriesVec GetTopmostCountries(ms::LatLon const & latlon) const = 0;
  };

  explicit NotificationManager(Delegate const & delegate);

  void Load();
  void TrimExpired();

  boost::optional<NotificationCandidate> GetNotification();

  // eye::Subscriber overrides:
  void OnMapObjectEvent(eye::MapObject const & poi) override;

private:
  bool Save();
  void ProcessUgcRateCandidates(eye::MapObject const & poi);
  Candidates::iterator GetUgcRateCandidate();

  Delegate const & m_delegate;
  // Notification candidates queue.
  Queue m_queue;
};
}  // namespace notifications
namespace lightweight
{
class NotificationManager
{
public:
  NotificationManager() : m_manager(m_delegate) { m_manager.Load(); }

  boost::optional<notifications::NotificationCandidate> GetNotification()
  {
    return m_manager.GetNotification();
  }

private:
  class EmptyDelegate : public notifications::NotificationManager::Delegate
  {
  public:
    // NotificationManager::Delegate overrides:
    storage::TCountriesVec GetTopmostCountries(ms::LatLon const & latlon) const override
    {
      return {};
    }
  };

  EmptyDelegate m_delegate;
  notifications::NotificationManager m_manager;
};
}  // namespace lightweight
