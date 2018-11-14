#include "testing/testing.hpp"

#include "map/notifications/notification_manager.hpp"
#include "map/notifications/notification_queue.hpp"
#include "map/notifications/notification_queue_serdes.hpp"
#include "map/notifications/notification_queue_storage.hpp"

#include "metrics/eye_info.hpp"

#include "platform/platform.hpp"

#include <memory>
#include <utility>

using namespace notifications;

namespace notifications
{
class NotificationManagerForTesting : public NotificationManager
{
public:
  explicit NotificationManagerForTesting(NotificationManager::Delegate & delegate)
    : NotificationManager(delegate)
  {
  }

  Queue & GetEditableQueue() { return m_queue; }

  void OnMapObjectEvent(eye::MapObject const & poi) override
  {
    ProcessUgcRateCandidates(poi);
  }
};
}  // namespace notifications

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

class DelegateForTesting : public NotificationManager::Delegate
{
public:
  // NotificationManager::Delegate overrides:
  ugc::Api * GetUGCApi() override
  {
    return nullptr;
  }
};

Queue MakeDefaultQueueForTesting()
{
  Queue queue;

  {
    NotificationCandidate notification;
    notification.m_type = NotificationCandidate::Type::UgcReview;

    notification.m_mapObject = std::make_unique<eye::MapObject>();
    notification.m_mapObject->SetBestType("cafe");
    notification.m_mapObject->SetPos({15.686299, 73.704084});
    notification.m_mapObject->SetReadableName("Baba");

    queue.m_candidates.emplace_back(std::move(notification));
  }

  {
    NotificationCandidate notification;
    notification.m_type = NotificationCandidate::Type::UgcReview;

    notification.m_mapObject = std::make_unique<eye::MapObject>();
    notification.m_mapObject->SetBestType("shop");
    notification.m_mapObject->SetPos({12.923975, 100.776627});
    notification.m_mapObject->SetReadableName("7eleven");

    queue.m_candidates.emplace_back(std::move(notification));
  }

  {
    NotificationCandidate notification;
    notification.m_type = NotificationCandidate::Type::UgcReview;

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

UNIT_CLASS_TEST(ScopedNotificationsQueue, Notifications_UgcRateCheckRouteToInSameGeoTrigger)
{
  DelegateForTesting delegate;
  NotificationManagerForTesting notificationManager(delegate);

  eye::MapObject mapObject;
  mapObject.SetPos(MercatorBounds::FromLatLon({59.909299, 10.769807}));
  mapObject.SetReadableName("Visiting a Bjarne");
  mapObject.SetBestType("amenity-bar");

  eye::MapObject::Event event;
  event.m_type = eye::MapObject::Event::Type::RouteToCreated;
  event.m_userPos = MercatorBounds::FromLatLon({59.920333, 10.780793});
  event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
  mapObject.GetEditableEvents().push_back(event);
  notificationManager.OnMapObjectEvent(mapObject);

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 1, ());
  notificationManager.GetEditableQueue().m_candidates[0].m_created = event.m_eventTime;

  auto result = notificationManager.GetNotification();

  TEST(result.is_initialized(), ());
  TEST_EQUAL(result.get().m_type, NotificationCandidate::Type::UgcReview, ());

  result = notificationManager.GetNotification();
  TEST(!result.is_initialized(), ());
}

UNIT_CLASS_TEST(ScopedNotificationsQueue, Notifications_UgcRateCheckUgcNotSavedTrigger)
{
  DelegateForTesting delegate;
  NotificationManagerForTesting notificationManager(delegate);

  eye::MapObject mapObject;
  mapObject.SetPos(MercatorBounds::FromLatLon({59.909299, 10.769807}));
  mapObject.SetReadableName("Visiting a Bjarne");
  mapObject.SetBestType("amenity-bar");

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::Open;
    event.m_userPos = MercatorBounds::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 0, ());

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::UgcEditorOpened;
    event.m_userPos = MercatorBounds::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 1, ());

  auto result = notificationManager.GetNotification();

  TEST(!result.is_initialized(), ());

  notificationManager.GetEditableQueue().m_candidates[0].m_created =
      notifications::Clock::now() - std::chrono::hours(25);

  result = notificationManager.GetNotification();

  TEST(result.is_initialized(), ());
  TEST_EQUAL(result.get().m_type, NotificationCandidate::Type::UgcReview, ());

  result = notificationManager.GetNotification();
  TEST(!result.is_initialized(), ());

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::UgcEditorOpened;
    event.m_userPos = MercatorBounds::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::UgcSaved;
    event.m_userPos = MercatorBounds::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  result = notificationManager.GetNotification();
  TEST(!result.is_initialized(), ());
}

UNIT_CLASS_TEST(ScopedNotificationsQueue, Notifications_UgcRateCheckPlannedTripTrigger)
{
  DelegateForTesting delegate;
  NotificationManagerForTesting notificationManager(delegate);

  eye::MapObject mapObject;
  mapObject.SetPos(MercatorBounds::FromLatLon({59.909299, 10.769807}));
  mapObject.SetReadableName("Visiting a Bjarne");
  mapObject.SetBestType("amenity-bar");

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::Open;
    event.m_userPos = MercatorBounds::FromLatLon({54.637300, 19.877731});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 0, ());

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::Open;
    event.m_userPos = MercatorBounds::FromLatLon({54.637310, 19.877735});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 0, ());

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::Open;
    event.m_userPos = MercatorBounds::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 1, ());

  auto result = notificationManager.GetNotification();

  TEST(!result.is_initialized(), ());

  notificationManager.GetEditableQueue().m_candidates[0].m_created =
      notifications::Clock::now() - std::chrono::hours(25);

  result = notificationManager.GetNotification();

  TEST(result.is_initialized(), ());
  TEST_EQUAL(result.get().m_type, NotificationCandidate::Type::UgcReview, ());

  result = notificationManager.GetNotification();
  TEST(!result.is_initialized(), ());
}
}  // namespace
