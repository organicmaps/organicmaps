#include "drape_frontend/read_mwm_task.hpp"

namespace df
{
ReadMWMTask::ReadMWMTask(MapDataProvider & model) : m_model(model)
{
#ifdef DEBUG
  m_checker = false;
#endif
}

void ReadMWMTask::Init(std::shared_ptr<TileInfo> const & tileInfo)
{
  m_tileInfo = tileInfo;
  m_tileKey = tileInfo->GetTileKey();
#ifdef DEBUG
  m_checker = true;
#endif
}

void ReadMWMTask::Reset()
{
#ifdef DEBUG
  m_checker = false;
#endif
  m_tileInfo.reset();
  IRoutine::Reset();
}

bool ReadMWMTask::IsCancelled() const
{
  std::shared_ptr<TileInfo> tile = m_tileInfo.lock();
  if (tile == nullptr)
    return true;

  return tile->IsCancelled() || IRoutine::IsCancelled();
}

void ReadMWMTask::Do()
{
#ifdef DEBUG
  ASSERT(m_checker, ());
#endif

  std::shared_ptr<TileInfo> tile = m_tileInfo.lock();
  if (tile == nullptr)
    return;
  try
  {
    tile->ReadFeatures(m_model);
  }
  catch (TileInfo::ReadCanceledException &)
  {
    return;
  }
}
}  // namespace df
