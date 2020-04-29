#include "testing/testing.hpp"

#include "map/notifications/notification_manager.hpp"
#include "map/notifications/notification_queue.hpp"
#include "map/notifications/notification_queue_serdes.hpp"
#include "map/notifications/notification_queue_storage.hpp"

#include "ugc/api.hpp"

#include "metrics/eye_info.hpp"

#include "storage/storage_defines.hpp"

#include "platform/platform.hpp"

#include <memory>
#include <string>
#include <utility>

using namespace notifications;
using namespace std::chrono;

namespace notifications
{
class NotificationManagerForTesting : public NotificationManager
{
public:
  class NotificationManagerDelegate : public NotificationManager::Delegate
  {
  public:
    ugc::Api & GetUGCApi() override
    {
      UNREACHABLE();
    }

    std::unordered_set<storage::CountryId> GetDescendantCountries(
        storage::CountryId const & country) const override
    {
      return {{"South Korea_North"}, {"South Korea_South"}};
    }

    storage::CountryId GetCountryAtPoint(m2::PointD const & pt) const override
    {
      return "South Korea_North";
    }

    std::string GetAddress(m2::PointD const & pt) override
    {
      return {};
    }
  };

  NotificationManagerForTesting()
  {
    SetDelegate(std::make_unique<NotificationManagerDelegate>());
  }

  Queue & GetEditableQueue() { return m_queue; }

  void OnMapObjectEvent(eye::MapObject const & poi) override
  {
    ProcessUgcRateCandidates(poi);
  }

  static void SetCreatedTime(NotificationCandidate & dst, Time time)
  {
    dst.m_created = time;
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

Queue MakeDefaultQueueForTesting()
{
  Queue queue;

  {
    eye::MapObject mapObject;
    mapObject.SetBestType("cafe");
    mapObject.SetPos({15.686299, 73.704084});
    mapObject.SetReadableName("Baba");

    queue.m_candidates.emplace_back(mapObject, "");
  }

  {
    eye::MapObject mapObject;
    mapObject.SetBestType("shop");
    mapObject.SetPos({12.923975, 100.776627});
    mapObject.SetReadableName("7eleven");

    queue.m_candidates.emplace_back(mapObject, "");
  }

  {
    eye::MapObject mapObject;
    mapObject.SetBestType("viewpoint");
    mapObject.SetPos({-45.943995, 167.619933});
    mapObject.SetReadableName("Waiau");

    queue.m_candidates.emplace_back(mapObject, "");
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
    TEST_EQUAL(lhsItem.GetType(), rhsItem.GetType(), ());
    TEST_EQUAL(lhsItem.GetBestFeatureType(), rhsItem.GetBestFeatureType(), ());
    TEST_EQUAL(lhsItem.GetReadableName(), rhsItem.GetReadableName(), ());
    TEST_EQUAL(lhsItem.GetPos(), rhsItem.GetPos(), ());
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
  NotificationManagerForTesting notificationManager;

  eye::MapObject mapObject;
  mapObject.SetPos(mercator::FromLatLon({59.909299, 10.769807}));
  mapObject.SetReadableName("Visiting a Bjarne");
  mapObject.SetBestType("amenity-bar");

  eye::MapObject::Event event;
  event.m_type = eye::MapObject::Event::Type::RouteToCreated;
  event.m_userPos = mercator::FromLatLon({59.920333, 10.780793});
  event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
  mapObject.GetEditableEvents().push_back(event);
  notificationManager.OnMapObjectEvent(mapObject);

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 1, ());

  auto & candidate = notificationManager.GetEditableQueue().m_candidates[0];
  NotificationManagerForTesting::SetCreatedTime(candidate, event.m_eventTime);

  auto result = notificationManager.GetNotification();

  TEST(result.has_value(), ());
  TEST_EQUAL(result->GetType(), NotificationCandidate::Type::UgcReview, ());

  result = notificationManager.GetNotification();
  TEST(!result.has_value(), ());
}

UNIT_CLASS_TEST(ScopedNotificationsQueue, Notifications_UgcRateCheckUgcNotSavedTrigger)
{
  NotificationManagerForTesting notificationManager;

  eye::MapObject mapObject;
  mapObject.SetPos(mercator::FromLatLon({59.909299, 10.769807}));
  mapObject.SetReadableName("Visiting a Bjarne");
  mapObject.SetBestType("amenity-bar");

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::Open;
    event.m_userPos = mercator::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 0, ());

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::UgcEditorOpened;
    event.m_userPos = mercator::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 1, ());

  auto result = notificationManager.GetNotification();

  TEST(!result.has_value(), ());

  auto & candidate = notificationManager.GetEditableQueue().m_candidates[0];
  NotificationManagerForTesting::SetCreatedTime(candidate, Clock::now() - hours(25));

  result = notificationManager.GetNotification();

  TEST(result.has_value(), ());
  TEST_EQUAL(result->GetType(), NotificationCandidate::Type::UgcReview, ());

  result = notificationManager.GetNotification();
  TEST(!result.has_value(), ());

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::UgcEditorOpened;
    event.m_userPos = mercator::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::UgcSaved;
    event.m_userPos = mercator::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  result = notificationManager.GetNotification();
  TEST(!result.has_value(), ());
}

UNIT_CLASS_TEST(ScopedNotificationsQueue, Notifications_UgcRateCheckPlannedTripTrigger)
{
  NotificationManagerForTesting notificationManager;

  eye::MapObject mapObject;
  mapObject.SetPos(mercator::FromLatLon({59.909299, 10.769807}));
  mapObject.SetReadableName("Visiting a Bjarne");
  mapObject.SetBestType("amenity-bar");

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::Open;
    event.m_userPos = mercator::FromLatLon({54.637300, 19.877731});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 0, ());

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::Open;
    event.m_userPos = mercator::FromLatLon({54.637310, 19.877735});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 0, ());

  {
    eye::MapObject::Event event;
    event.m_type = eye::MapObject::Event::Type::Open;
    event.m_userPos = mercator::FromLatLon({59.920333, 10.780793});
    event.m_eventTime = notifications::Clock::now() - std::chrono::hours(25);
    mapObject.GetEditableEvents().push_back(event);
    notificationManager.OnMapObjectEvent(mapObject);
  }

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 1, ());

  auto result = notificationManager.GetNotification();

  TEST(!result.has_value(), ());

  auto & candidate = notificationManager.GetEditableQueue().m_candidates[0];
  NotificationManagerForTesting::SetCreatedTime(candidate, Clock::now() - hours(25));

  result = notificationManager.GetNotification();

  TEST(result.has_value(), ());
  TEST_EQUAL(result->GetType(), NotificationCandidate::Type::UgcReview, ());

  result = notificationManager.GetNotification();
  TEST(!result.has_value(), ());
}

UNIT_CLASS_TEST(ScopedNotificationsQueue, Notifications_DeleteCanidatesForCountry)
{
  NotificationManagerForTesting notificationManager;

  eye::MapObject mapObject;
  mapObject.SetPos(mercator::FromLatLon({35.966941, 127.571227}));
  mapObject.SetReadableName("Joseon");
  mapObject.SetBestType("amenity-bar");

  notificationManager.GetEditableQueue().m_candidates.push_back({mapObject, ""});

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 1, ());
  notificationManager.DeleteCandidatesForCountry("South Korea_North");
  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 0, ());

  notificationManager.GetEditableQueue().m_candidates.push_back({mapObject, ""});

  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 1, ());
  notificationManager.DeleteCandidatesForCountry("South Korea");
  TEST_EQUAL(notificationManager.GetEditableQueue().m_candidates.size(), 0, ());
}
}  // namespace
