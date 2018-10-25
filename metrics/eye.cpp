#include "metrics/eye.hpp"
#include "metrics/eye_serdes.hpp"
#include "metrics/eye_storage.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/assert.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

using namespace eye;

namespace
{
void Load(Info & info)
{
  Storage::Migrate();

  std::vector<int8_t> infoFileData;
  std::vector<int8_t> mapObjectsFileData;
  if (!Storage::LoadInfo(infoFileData) || !Storage::LoadMapObjects(mapObjectsFileData))
  {
    info = {};
    return;
  }

  try
  {
    Serdes::DeserializeInfo(infoFileData, info);
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

// TODO: use to trim old map object events.
//bool SaveMapObjects(Info const & info)
//{
//  std::vector<int8_t> fileData;
//  Serdes::SerializeMapObjects(info.m_mapObjects, fileData);
//  return Storage::SaveMapObjects(fileData);
//}
//
// TODO: use it to save map object events with append only flag.
//bool SaveMapObjectEvent(MapObject const & mapObject, MapObject::Event const & event)
//{
//  std::vector<int8_t> eventData;
//  Serdes::SerializeMapObjectEvent(mapObject, event, eventData);
//
//  return Storage::AppendMapObjectEvent(eventData);
//}
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
  GetPlatform().RunTask(Platform::Thread::File, [this, subscriber]
  {
    m_subscribers.push_back(subscriber);
  });
}

void Eye::Save(InfoType const & info)
{
  if (::Save(*info))
    m_info.Set(info);
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
    editableTips.emplace_back(tip);
  }

  Save(editableInfo);

  for (auto subscriber : m_subscribers)
  {
    subscriber->OnTipClicked(tip);
  }
}

void Eye::UpdateBookingFilterUsedTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto const now = Clock::now();

  editableInfo->m_booking.m_lastFilterUsedTime = now;

  Save(editableInfo);

  for (auto subscriber : m_subscribers)
  {
    subscriber->OnBookingFilterUsed(now);
  }
}

void Eye::UpdateBoomarksCatalogShownTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto const now = Clock::now();

  editableInfo->m_bookmarks.m_lastOpenedTime = now;

  Save(editableInfo);

  for (auto subscriber : m_subscribers)
  {
    subscriber->OnBookmarksCatalogShown(now);
  }
}

void Eye::UpdateDiscoveryShownTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto const now = Clock::now();

  editableInfo->m_discovery.m_lastOpenedTime = now;

  Save(editableInfo);

  for (auto subscriber : m_subscribers)
  {
    subscriber->OnDiscoveryShown(now);
  }
}

void Eye::IncrementDiscoveryItem(Discovery::Event event)
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);

  editableInfo->m_discovery.m_lastClickedTime = Clock::now();
  editableInfo->m_discovery.m_eventCounters.Increment(event);

  Save(editableInfo);

  for (auto subscriber : m_subscribers)
  {
    subscriber->OnDiscoveryItemClicked(event);
  }
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

  Save(editableInfo);

  for (auto subscriber : m_subscribers)
  {
    subscriber->OnLayerUsed(layer);
  }
}

void Eye::RegisterPlacePageOpened()
{

}

void Eye::RegisterUgcEditorOpened()
{

}

void Eye::RegisterUgcSaved()
{

}

void Eye::RegisterAddToBookmarkClicked()
{

}

void Eye::RegisterRouteCreatedToObject()
{

}

// Eye::Event methods ------------------------------------------------------------------------------
// static
void Eye::Event::TipClicked(Tip::Type type, Tip::Event event)
{
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
void Eye::Event::PlacePageOpened()
{
  GetPlatform().RunTask(Platform::Thread::File, []
  {
    Instance().RegisterPlacePageOpened();
  });
}

// static
void Eye::Event::UgcEditorOpened()
{
  GetPlatform().RunTask(Platform::Thread::File, []
  {
    Instance().RegisterUgcEditorOpened();
  });
}

//static
void Eye::Event::UgcSaved()
{
  GetPlatform().RunTask(Platform::Thread::File, []
  {
    Instance().RegisterUgcSaved();
  });
}

// static
void Eye::Event::AddToBookmarkClicked()
{
  GetPlatform().RunTask(Platform::Thread::File, []
  {
    Instance().RegisterAddToBookmarkClicked();
  });
}

// static
void Eye::Event::RouteCreatedToObject()
{
  GetPlatform().RunTask(Platform::Thread::File, []
  {
    Instance().RegisterRouteCreatedToObject();
  });
}
}  // namespace eye
