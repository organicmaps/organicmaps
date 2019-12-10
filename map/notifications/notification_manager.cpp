#include "map/notifications/notification_manager.hpp"

#include "map/notifications/notification_queue_serdes.hpp"
#include "map/notifications/notification_queue_storage.hpp"

#include "ugc/api.hpp"

#include "indexer/classificator.hpp"

#include "storage/storage_defines.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <utility>

using namespace notifications;

namespace
{
auto constexpr kCandidatesExpirePeriod = std::chrono::hours(24 * 30);
auto constexpr kPeriodBetweenNotifications = std::chrono::hours(24);
auto constexpr kMinTimeSinceLastEventForUgcRate = std::chrono::hours(24);

double constexpr kMinDistanceToTriggerUgcRateInMeters = 50000.0;  // 50 km

uint32_t constexpr kOpenCountForPlannedTripTrigger = 2;

bool CheckUgcNotSavedTrigger(eye::MapObject const & poi)
{
  bool ugcEditorShowed = false;
  bool ugcSaved = false;
  for (auto const & event : poi.GetEvents())
  {
    if (event.m_type == eye::MapObject::Event::Type::UgcEditorOpened && !ugcEditorShowed)
    {
      ugcEditorShowed = true;
    }
    else if (event.m_type == eye::MapObject::Event::Type::UgcSaved)
    {
      ugcSaved = true;
      break;
    }
  }

  if (ugcEditorShowed && !ugcSaved)
    return true;

  return false;
}

bool IsSmallDistance(eye::MapObject const & poi, eye::MapObject::Event const & event)
{
  auto const distanceToUser = mercator::DistanceOnEarth(event.m_userPos, poi.GetPos());
  return distanceToUser <= kMinDistanceToTriggerUgcRateInMeters;
}

bool CheckRouteToInSameGeoTrigger(eye::MapObject const & poi)
{
  CHECK_GREATER(poi.GetEvents().size(), 0, ());

  auto const & lastEvent = poi.GetEvents().back();

  if (lastEvent.m_type != eye::MapObject::Event::Type::RouteToCreated)
    return false;

  return IsSmallDistance(poi, lastEvent);
}

bool CheckPlannedTripTrigger(eye::MapObject const & poi)
{
  CHECK_GREATER(poi.GetEvents().size(), 0, ());

  auto const & events = poi.GetEvents();

  if (events.back().m_type != eye::MapObject::Event::Type::Open)
    return false;

  if (!IsSmallDistance(poi, events.back()))
    return false;

  uint32_t openCounter = 0;
  for (size_t i = 0; i < events.size() - 1; ++i)
  {
    if (events[i].m_type != eye::MapObject::Event::Type::Open &&
        events[i].m_type != eye::MapObject::Event::Type::AddToBookmark &&
        events[i].m_type != eye::MapObject::Event::Type::RouteToCreated)
    {
      continue;
    }

    if (IsSmallDistance(poi, events[i]))
      continue;

    if (events[i].m_type == eye::MapObject::Event::Type::Open)
      ++openCounter;
    else
      return true;

    if (openCounter >= kOpenCountForPlannedTripTrigger)
      return true;
  }

  return false;
}
}  // namespace

namespace notifications
{
void NotificationManager::SetDelegate(std::unique_ptr<Delegate> delegate)
{
  m_delegate = std::move(delegate);
}

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

void NotificationManager::TrimExpired()
{
  auto & candidates = m_queue.m_candidates;
  size_t const sizeBefore = candidates.size();

  candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [](auto const & item)
  {
    if (item.IsUsed())
      return Clock::now() - item.GetLastUsedTime() >= eye::Eye::GetMapObjectEventsExpirePeriod();

    return Clock::now() - item.GetCreatedTime() >= kCandidatesExpirePeriod;
  }), candidates.end());

  if (sizeBefore != candidates.size())
    VERIFY(Save(), ());
}

Notification NotificationManager::GetNotification()
{
  if (Clock::now() - m_queue.m_lastNotificationProvidedTime < kPeriodBetweenNotifications)
    return {};

  auto & candidates = m_queue.m_candidates;

  if (candidates.empty())
    return {};

  auto it = GetUgcRateCandidate();

  if (it == candidates.end())
    return {};

  it->MarkAsUsed();
  m_queue.m_lastNotificationProvidedTime = Clock::now();

  VERIFY(Save(), ());

  return *it;
}

size_t NotificationManager::GetCandidatesCount() const
{
  return m_queue.m_candidates.size();
}

void NotificationManager::DeleteCandidatesForCountry(storage::CountryId const & countryId)
{
  auto const countries = m_delegate->GetDescendantCountries(countryId);

  auto & candidates = m_queue.m_candidates;
  size_t const sizeBefore = candidates.size();

  base::EraseIf(candidates, [this, &countries](auto const & item)
  {
    auto const itemCountry = m_delegate->GetCountryAtPoint(item.GetPos());
    return countries.count(itemCountry) != 0;
  });

  if (sizeBefore != candidates.size())
    VERIFY(Save(), ());
}

void NotificationManager::OnMapObjectEvent(eye::MapObject const & poi)
{
  CHECK_GREATER(poi.GetEvents().size(), 0, ());

  if (poi.GetEvents().back().m_type == eye::MapObject::Event::Type::UgcSaved)
    return ProcessUgcRateCandidates(poi);

  auto const bestType = classif().GetTypeByReadableObjectName(poi.GetBestType());
  m_delegate->GetUGCApi().HasUGCForPlace(bestType, poi.GetPos(), [this, poi] (bool result)
  {
    if (!result)
      ProcessUgcRateCandidates(poi);
  });
}

bool NotificationManager::Save()
{
  std::vector<int8_t> fileData;
  QueueSerdes::Serialize(m_queue, fileData);
  return QueueStorage::Save(fileData);
}

void NotificationManager::ProcessUgcRateCandidates(eye::MapObject const & poi)
{
  CHECK_GREATER(poi.GetEvents().size(), 0, ());

  if (poi.IsEmpty())
    return;

  auto it = m_queue.m_candidates.begin();
  for (; it != m_queue.m_candidates.end(); ++it)
  {
    if (it->GetType() != NotificationCandidate::Type::UgcReview || it->IsUsed())
      continue;

    if (it->IsSameMapObject(poi))
    {
      if (poi.GetEvents().back().m_type == eye::MapObject::Event::Type::UgcSaved)
      {
        m_queue.m_candidates.erase(it);
        VERIFY(Save(), ());
      }

      return;
    }
  }

  if (poi.GetEvents().back().m_type == eye::MapObject::Event::Type::UgcSaved)
    return;

  if (CheckUgcNotSavedTrigger(poi) || CheckRouteToInSameGeoTrigger(poi) ||
      CheckPlannedTripTrigger(poi))
  {
    m_queue.m_candidates.emplace_back(poi, m_delegate->GetAddress(poi.GetPos()));

    VERIFY(Save(), ());
  }
}

Candidates::iterator NotificationManager::GetUgcRateCandidate()
{
  auto it = m_queue.m_candidates.begin();
  for (; it != m_queue.m_candidates.end(); ++it)
  {
    if (!it->IsUsed() && it->GetType() == NotificationCandidate::Type::UgcReview &&
        Clock::now() - it->GetCreatedTime() >= kMinTimeSinceLastEventForUgcRate)
    {
      return it;
    }
  }

  return it;
}
}  // namespace notifications
