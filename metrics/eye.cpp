#include "metrics/eye.hpp"
#include "metrics/eye_serdes.hpp"
#include "metrics/eye_storage.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <vector>

using namespace eye;

namespace
{
// Three months.
auto constexpr kMapObjectEventsExpirePeriod = std::chrono::hours(24 * 30 * 3);
auto constexpr kEventCooldown = std::chrono::seconds(2);

std::array<std::string, 7> const kMapEventSupportedTypes = {{"amenity-bar", "amenity-cafe",
                                                            "amenity-pub", "amenity-restaurant",
                                                            "amenity-fast_food", "amenity-biergarden",
                                                            "shop-bakery"}};

void Load(Info & info)
{
  Storage::Migrate();

  std::vector<int8_t> infoFileData;
  std::vector<int8_t> mapObjectsFileData;
  if (!Storage::LoadInfo(infoFileData) && !Storage::LoadMapObjects(mapObjectsFileData))
  {
    info = {};
    return;
  }

  try
  {
    if (!infoFileData.empty())
      Serdes::DeserializeInfo(infoFileData, info);

    // Workaround to remove tips which were saved by mistake.
    info.m_tips.erase(std::remove_if(info.m_tips.begin(), info.m_tips.end(), [](auto const & item)
    {
      return item.m_type == Tip::Type::Count;
    }), info.m_tips.end());

    if (!mapObjectsFileData.empty())
      Serdes::DeserializeMapObjects(mapObjectsFileData, info.m_mapObjects);
  }
  catch (Serdes::UnknownVersion const & ex)
  {
    LOG(LERROR, ("Cannot load metrics files, eye will be disabled. Exception:", ex.Msg()));
    info = {};
  }
}

bool Save(Info const & info)
{
  std::vector<int8_t> fileData;
  Serdes::SerializeInfo(info, fileData);
  return Storage::SaveInfo(fileData);
}

bool SaveMapObjects(MapObjects const & mapObjects)
{
  std::vector<int8_t> fileData;
  Serdes::SerializeMapObjects(mapObjects, fileData);
  return Storage::SaveMapObjects(fileData);
}

bool SaveLastMapObjectEvent(MapObject const & mapObject)
{
  ASSERT(!mapObject.GetEvents().empty(), ());

  std::vector<int8_t> eventData;
  Serdes::SerializeMapObjectEvent(mapObject, mapObject.GetEvents().back(), eventData);

  return Storage::AppendMapObjectEvent(eventData);
}
}  // namespace

namespace eye
{
Eye::Eye()
{
  Info info;
  Load(info);
  m_info.Set(std::make_shared<Info>(info));
}

// static
Eye & Eye::Instance()
{
  static Eye instance;
  return instance;
}

Eye::InfoType Eye::GetInfo() const
{
  return m_info.Get();
}

void Eye::Subscribe(Subscriber * subscriber)
{
  m_subscribers.push_back(subscriber);
}

void Eye::UnsubscribeAll()
{
  m_subscribers.clear();
}

// static
std::chrono::hours const & Eye::GetMapObjectEventsExpirePeriod()
{
  return kMapObjectEventsExpirePeriod;
}

void Eye::TrimExpired()
{
  GetPlatform().RunTask(Platform::Thread::File, [this]
  {
    TrimExpiredMapObjectEvents();
  });
}

bool Eye::Save(InfoType const & info)
{
  if (!::Save(*info))
    return false;

  m_info.Set(info);
  return true;
}

void Eye::TrimExpiredMapObjectEvents()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto changed = false;

  std::vector<MapObject> removeQueue;

  editableInfo->m_mapObjects.ForEach([&removeQueue, &changed](MapObject const & item)
  {
    auto & events = item.GetEditableEvents();
    events.erase(std::remove_if(events.begin(), events.end(), [&changed](auto const & item)
    {
      if (Clock::now() - item.m_eventTime >= kMapObjectEventsExpirePeriod)
      {
        if (!changed)
          changed = true;

        return true;
      }
      return false;
    }), events.end());

    if (events.empty())
      removeQueue.push_back(item);
  });

  for (auto const & toRemove : removeQueue)
  {
    editableInfo->m_mapObjects.Erase(toRemove);
  }

  if (changed && SaveMapObjects(editableInfo->m_mapObjects))
    m_info.Set(editableInfo);
}

void Eye::RegisterTipClick(Tip::Type type, Tip::Event event)
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto & editableTips = editableInfo->m_tips;

  auto it = std::find_if(editableTips.begin(), editableTips.end(), [type](Tip const & tip)
  {
    return tip.m_type == type;
  });

  Tip tip;
  auto const now = Clock::now();
  if (it != editableTips.cend())
  {
    it->m_eventCounters.Increment(event);
    it->m_lastShownTime = now;
    tip = *it;
  }
  else
  {
    tip.m_type = type;
    tip.m_eventCounters.Increment(event);
    tip.m_lastShownTime = now;
    editableTips.push_back(tip);
  }

  if (!Save(editableInfo))
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, tip]
  {
    for (auto subscriber : m_subscribers)
    {
      subscriber->OnTipClicked(tip);
    }
  });
}

void Eye::UpdateBookingFilterUsedTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto const now = Clock::now();

  editableInfo->m_booking.m_lastFilterUsedTime = now;

  if (!Save(editableInfo))
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, now]
  {
    for (auto subscriber : m_subscribers)
    {
      subscriber->OnBookingFilterUsed(now);
    }
  });
}

void Eye::UpdateBoomarksCatalogShownTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto const now = Clock::now();

  editableInfo->m_bookmarks.m_lastOpenedTime = now;

  if (!Save(editableInfo))
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, now]
  {
    for (auto subscriber : m_subscribers)
    {
      subscriber->OnBookmarksCatalogShown(now);
    }
  });
}

void Eye::UpdateDiscoveryShownTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto const now = Clock::now();

  editableInfo->m_discovery.m_lastOpenedTime = now;

  if (!Save(editableInfo))
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, now]
  {
    for (auto subscriber : m_subscribers)
    {
      subscriber->OnDiscoveryShown(now);
    }
  });
}

void Eye::IncrementDiscoveryItem(Discovery::Event event)
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);

  editableInfo->m_discovery.m_lastClickedTime = Clock::now();
  editableInfo->m_discovery.m_eventCounters.Increment(event);

  if (!Save(editableInfo))
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, event]
  {
    for (auto subscriber : m_subscribers)
    {
      subscriber->OnDiscoveryItemClicked(event);
    }
  });
}

void Eye::RegisterLayerShown(Layer::Type type)
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto & editableLayers = editableInfo->m_layers;

  auto it = std::find_if(editableLayers.begin(), editableLayers.end(), [type](Layer const & layer)
  {
    return layer.m_type == type;
  });

  Layer layer;
  if (it != editableLayers.end())
  {
    ++it->m_useCount;
    it->m_lastTimeUsed = Clock::now();
    layer = *it;
  }
  else
  {
    layer.m_type = type;

    ++layer.m_useCount;
    layer.m_lastTimeUsed = Clock::now();
    editableLayers.emplace_back(layer);
  }

  if (!Save(editableInfo))
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, layer]
  {
    for (auto subscriber : m_subscribers)
    {
      subscriber->OnLayerShown(layer);
    }
  });
}

void Eye::RegisterMapObjectEvent(MapObject const & mapObject, MapObject::Event::Type type,
                                 m2::PointD const & userPos)
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto & mapObjects = editableInfo->m_mapObjects;

  MapObject result = mapObject;
  MapObject::Event event;
  event.m_type = type;
  event.m_userPos = userPos;
  event.m_eventTime = Clock::now();

  bool found = false;
  bool duplication = false;
  mapObjects.ForEachInRect(
    result.GetLimitRect(), [&found, &duplication, &event, &result](MapObject const & item)
  {
    if (found || duplication || !item.AlmostEquals(result))
      return;

    if (!found)
      found = true;

    auto & events = item.GetEditableEvents();
    if (!events.empty() && events.back().m_type == event.m_type &&
        event.m_eventTime - events.back().m_eventTime <= kEventCooldown)
    {
      duplication = true;
    }

    item.GetEditableEvents().emplace_back(std::move(event));
    result = item;
  });

  if (duplication)
    return;

  if (!found)
  {
    result.GetEditableEvents() = {std::move(event)};
    mapObjects.Add(result);
  }

  if (!SaveLastMapObjectEvent(result))
    return;

  m_info.Set(editableInfo);
  GetPlatform().RunTask(Platform::Thread::Gui, [this, result]
  {
    for (auto subscriber : m_subscribers)
    {
      subscriber->OnMapObjectEvent(result);
    }
  });
}

// Eye::Event methods ------------------------------------------------------------------------------
// static
void Eye::Event::TipClicked(Tip::Type type, Tip::Event event)
{
  CHECK_NOT_EQUAL(type, Tip::Type::Count, ());
  CHECK_NOT_EQUAL(event, Tip::Event::Count, ());

  GetPlatform().RunTask(Platform::Thread::File, [type, event]
  {
    Instance().RegisterTipClick(type, event);
  });
}

// static
void Eye::Event::BookingFilterUsed()
{
  GetPlatform().RunTask(Platform::Thread::File, []
  {
    Instance().UpdateBookingFilterUsedTime();
  });
}

// static
void Eye::Event::BoomarksCatalogShown()
{
  GetPlatform().RunTask(Platform::Thread::File, []
  {
    Instance().UpdateBoomarksCatalogShownTime();
  });
}

// static
void Eye::Event::DiscoveryShown()
{
  GetPlatform().RunTask(Platform::Thread::File, []
  {
    Instance().UpdateDiscoveryShownTime();
  });
}

// static
void Eye::Event::DiscoveryItemClicked(Discovery::Event event)
{
  CHECK_NOT_EQUAL(event, Discovery::Event::Count, ());

  GetPlatform().RunTask(Platform::Thread::File, [event]
  {
    Instance().IncrementDiscoveryItem(event);
  });
}

// static
void Eye::Event::LayerShown(Layer::Type type)
{
  GetPlatform().RunTask(Platform::Thread::File, [type]
  {
    Instance().RegisterLayerShown(type);
  });
}

// static
void Eye::Event::MapObjectEvent(MapObject const & mapObject, MapObject::Event::Type type,
                                m2::PointD const & userPos)
{
  if (mapObject.GetReadableName().empty())
    return;

  {
    auto const it = std::find(kMapEventSupportedTypes.cbegin(), kMapEventSupportedTypes.cend(),
                              mapObject.GetBestType());
    if (it == kMapEventSupportedTypes.cend())
      return;
  }

  GetPlatform().RunTask(Platform::Thread::File, [type, mapObject, userPos]
  {
    Instance().RegisterMapObjectEvent(mapObject, type, userPos);
  });
}
}  // namespace eye
