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
  void Load();

  boost::optional<Notification> GetNotification() const;

  // eye::Subscriber overrides:
  void OnMapObjectEvent(eye::MapObject const & poi, eye::MapObject::Events const & events) override;

private:
  bool Save();

  // Notification candidates queue.
  Queue m_queue;
};
}  // namespace notifications
