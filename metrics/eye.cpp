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

// Eye::Event methods ------------------------------------------------------------------------------
// static
void Eye::Event::TipShown(Tip::Type type, Tip::Event event)
{
  GetPlatform().RunTask(Platform::Thread::File, [type, event]
  {
    Instance().AppendTip(type, event);
  });
}
}  // namespace eye
