#include "metrics/eye.hpp"
#include "metrics/eye_serdes.hpp"
#include "metrics/eye_storage.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

using namespace eye;

namespace
{
void Load(Info & info)
{
  std::vector<int8_t> fileData;
  if (!Storage::Load(Storage::GetEyeFilePath(), fileData))
  {
    info = {};
    return;
  }

  try
  {
    Serdes::Deserialize(fileData, info);
  }
  catch (Serdes::UnknownVersion const & ex)
  {
    LOG(LERROR, ("Unknown eye file version, eye will be disabled. Exception:", ex.Msg()));
  }
}

bool Save(Info const & info)
{
  std::vector<int8_t> fileData;
  Serdes::Serialize(info, fileData);
  return Storage::Save(Storage::GetEyeFilePath(), fileData);
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

void Eye::Save(InfoType const & info)
{
  if (::Save(*info))
    m_info.Set(info);
}

void Eye::AppendTip(Tip::Type type, Tip::Event event)
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto & editableTips = editableInfo->m_tips;

  auto it = std::find_if(editableTips.begin(), editableTips.end(), [type](Tip const & tip)
  {
    return tip.m_type == type;
  });

  auto const now = Clock::now();
  if (it != editableTips.cend())
  {
    it->m_eventCounters.Increment(event);
    it->m_lastShownTime = now;
  }
  else
  {
    Tip tip;
    tip.m_type = type;
    tip.m_eventCounters.Increment(event);
    tip.m_lastShownTime = now;
    editableTips.emplace_back(std::move(tip));
  }

  Save(editableInfo);
}

void Eye::UpdateBookingFilterUsedTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);

  editableInfo->m_booking.m_lastFilterUsedTime = Clock::now();

  Save(editableInfo);
}

void Eye::UpdateBoomarksCatalogShownTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);

  editableInfo->m_bookmarks.m_lastOpenedTime = Clock::now();

  Save(editableInfo);
}

void Eye::UpdateDiscoveryShownTime()
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);

  editableInfo->m_discovery.m_lastOpenedTime = Clock::now();

  Save(editableInfo);
}

void Eye::IncrementDiscoveryItem(Discovery::Event event)
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);

  editableInfo->m_discovery.m_lastClickedTime = Clock::now();
  editableInfo->m_discovery.m_eventCounters.Increment(event);

  Save(editableInfo);
}

void Eye::AppendLayer(Layer::Type type)
{
  auto const info = m_info.Get();
  auto editableInfo = std::make_shared<Info>(*info);
  auto & editableLayers = editableInfo->m_layers;

  auto it = std::find_if(editableLayers.begin(), editableLayers.end(), [type](Layer const & layer)
  {
    return layer.m_type == type;
  });

  if (it != editableLayers.end())
  {
    ++it->m_useCount;
    it->m_lastTimeUsed = Clock::now();
  }
  else
  {
    Layer layer;
    layer.m_type = type;

    ++layer.m_useCount;
    layer.m_lastTimeUsed = Clock::now();
    editableLayers.emplace_back(std::move(layer));
  }

  Save(editableInfo);
}

// Eye::Event methods ------------------------------------------------------------------------------
// static
void Eye::Event::TipShown(Tip::Type type, Tip::Event event)
{
  GetPlatform().RunTask(Platform::Thread::File, [type, event]
  {
    Instance().AppendTip(type, event);
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
void Eye::Event::LayerUsed(Layer::Type type)
{
  GetPlatform().RunTask(Platform::Thread::File, [type]
  {
    Instance().AppendLayer(type);
  });
}
}  // namespace eye
