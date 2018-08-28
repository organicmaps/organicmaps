#include "metrics/eye.hpp"
#include "metrics/eye_serdes.hpp"
#include "metrics/eye_storage.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "base/logging.hpp"

#include <algorithm>

using namespace eye;

namespace
{
void Load(Info & info)
{
  std::vector<int8_t> fileData;
  Storage::Load(Storage::GetEyeFilePath(), fileData);

  if (fileData.empty())
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

void Eye::AppendTip(Tips::Type type, Tips::Event event)
{
  auto const info = m_info.Get();
  auto editableInfo = make_shared<Info>(*info);
  auto & editableTips = editableInfo->m_tips;

  auto & shownTips = editableTips.m_shownTips;

  ++(editableTips.m_totalShownTipsCount);
  editableTips.m_lastShown = Clock::now();

  auto it = std::find_if(shownTips.begin(), shownTips.end(), [type](Tips::Info const & tipsInfo)
  {
    return tipsInfo.m_type == type;
  });

  if (it != shownTips.cend())
  {
    it->m_eventCounters.Increment(event);
    it->m_lastShown = editableTips.m_lastShown;
  }
  else
  {
    Tips::Info tipInfo;
    tipInfo.m_type = type;
    tipInfo.m_eventCounters.Increment(event);
    tipInfo.m_lastShown = editableTips.m_lastShown;
    shownTips.emplace_back(std::move(tipInfo));
  }

  Save(editableInfo);
}

// Eye::Event methods ------------------------------------------------------------------------------
// static
void Eye::Event::TipShown(Tips::Type type, Tips::Event event)
{
  Platform().RunTask(Platform::Thread::File, [type, event]
  {
    Instance().AppendTip(type, event);
  });
}
}  // namespace eye
