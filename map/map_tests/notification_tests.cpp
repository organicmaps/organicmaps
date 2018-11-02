#include "testing/testing.hpp"

#include "map/notifications/notification_queue.hpp"
#include "map/notifications/notification_queue_serdes.hpp"
#include "map/notifications/notification_queue_storage.hpp"

#include "metrics/eye_info.hpp"

#include "platform/platform.hpp"

#include <memory>
#include <utility>

using namespace notifications;

namespace
{
class ScopedNotificationsQueue
{
public:
  ~ScopedNotificationsQueue()
  {
    GetPlatform().RmDirRecursively(QueueStorage::GetNotificationsDir());
  }
};

Queue MakeDefaultQueueForTesting()
{
  Queue queue;

  {
    Notification notification;
    notification.m_type = Notification::Type::UgcReview;

    notification.m_mapObject = std::make_unique<eye::MapObject>();
    notification.m_mapObject->SetBestType("cafe");
    notification.m_mapObject->SetPos({15.686299, 73.704084});
    notification.m_mapObject->SetReadableName("Baba");

    queue.m_candidates.emplace_back(std::move(notification));
  }

  {
    Notification notification;
    notification.m_type = Notification::Type::UgcReview;

    notification.m_mapObject = std::make_unique<eye::MapObject>();
    notification.m_mapObject->SetBestType("shop");
    notification.m_mapObject->SetPos({12.923975, 100.776627});
    notification.m_mapObject->SetReadableName("7eleven");

    queue.m_candidates.emplace_back(std::move(notification));
  }

  {
    Notification notification;
    notification.m_type = Notification::Type::UgcReview;

    notification.m_mapObject = std::make_unique<eye::MapObject>();
    notification.m_mapObject->SetBestType("viewpoint");
    notification.m_mapObject->SetPos({-45.943995, 167.619933});
    notification.m_mapObject->SetReadableName("Waiau");

    queue.m_candidates.emplace_back(std::move(notification));
  }

  return queue;
}

void CompareWithDefaultQueue(Queue const & lhs)
{
  auto const rhs = MakeDefaultQueueForTesting();

  TEST_EQUAL(lhs.m_candidates.size(), rhs.m_candidates.size(), ());

  for (size_t i = 0; i < lhs.m_candidates.size(); ++i)
  {
    auto const & lhsItem = lhs.m_candidates[i];
    auto const & rhsItem = rhs.m_candidates[i];
    TEST_EQUAL(lhsItem.m_type, rhsItem.m_type, ());
    TEST(lhsItem.m_mapObject, ());
    TEST_EQUAL(lhsItem.m_mapObject->GetBestType(), rhsItem.m_mapObject->GetBestType(), ());
    TEST_EQUAL(lhsItem.m_mapObject->GetReadableName(), rhsItem.m_mapObject->GetReadableName(), ());
    TEST_EQUAL(lhsItem.m_mapObject->GetPos(), lhsItem.m_mapObject->GetPos(), ());
  }
}

UNIT_TEST(Notifications_QueueSerdesTest)
{
  auto const queue = MakeDefaultQueueForTesting();

  std::vector<int8_t> queueData;
  QueueSerdes::Serialize(queue, queueData);
  Queue result;
  QueueSerdes::Deserialize(queueData, result);

  CompareWithDefaultQueue(result);
}

UNIT_CLASS_TEST(ScopedNotificationsQueue, Notifications_QueueSaveLoadTest)
{
  auto const queue = MakeDefaultQueueForTesting();

  std::vector<int8_t> queueData;
  QueueSerdes::Serialize(queue, queueData);
  TEST(QueueStorage::Save(queueData), ());
  queueData.clear();
  TEST(QueueStorage::Load(queueData), ());
  Queue result;
  QueueSerdes::Deserialize(queueData, result);

  CompareWithDefaultQueue(result);
}
}  // namespace
