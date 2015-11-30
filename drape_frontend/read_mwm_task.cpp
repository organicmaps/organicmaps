#include "drape_frontend/read_mwm_task.hpp"

namespace df
{
ReadMWMTask::ReadMWMTask(MemoryFeatureIndex & memIndex, MapDataProvider & model)
  : m_memIndex(memIndex)
  , m_model(model)
{
#ifdef DEBUG
  m_checker = false;
#endif
}

void ReadMWMTask::Init(shared_ptr<TileInfo> const & tileInfo)
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
}

bool ReadMWMTask::IsCancelled() const
{
  shared_ptr<TileInfo> tile = m_tileInfo.lock();
  if (tile == nullptr)
    return true;

  return tile->IsCancelled() || IRoutine::IsCancelled();
}

void ReadMWMTask::Do()
{
#ifdef DEBUG
  ASSERT(m_checker, ());
#endif

  shared_ptr<TileInfo> tile = m_tileInfo.lock();
  if (tile == nullptr)
    return;
  try
  {
    tile->ReadFeatures(m_model, m_memIndex);
  }
  catch (TileInfo::ReadCanceledException &)
  {
    return;
  }
}

} // namespace df
