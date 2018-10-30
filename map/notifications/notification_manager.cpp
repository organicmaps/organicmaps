#include "map/notifications/notification_manager.hpp"
#include "map/notifications/notification_queue_serdes.hpp"
#include "map/notifications/notification_queue_storage.hpp"

#include "base/logging.hpp"

namespace notifications
{
void NotificationManager::Load()
{
  std::vector<int8_t> queueFileData;
  if (!QueueStorage::Load(queueFileData))
  {
    m_queue = {};
    return;
  }

  try
  {
    QueueSerdes::Deserialize(queueFileData, m_queue);
  }
  catch (QueueSerdes::UnknownVersion const & ex)
  {
    // Notifications queue might be empty.
    m_queue = {};
  }
}

boost::optional<Notification> NotificationManager::GetNotification() const
{
  if (m_queue.m_candidates.empty())
    return {};

  // Is not implemented yet. Coming soon.

  return {};
}

void NotificationManager::OnMapObjectEvent(eye::MapObject const & poi,
                                           eye::MapObject::Events const & events)
{
  // Is not implemented yet. Coming soon.
}

bool NotificationManager::Save()
{
  std::vector<int8_t> fileData;
  QueueSerdes::Serialize(m_queue, fileData);
  return QueueStorage::Save(fileData);
}
}  // namespace notifications
