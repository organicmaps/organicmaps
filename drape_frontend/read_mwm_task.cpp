#include "drape_frontend/read_mwm_task.hpp"

#include "std/shared_ptr.hpp"

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

void ReadMWMTask::Init(weak_ptr<TileInfo> const & tileInfo)
{
  m_tileInfo = tileInfo;
#ifdef DEBUG
  m_checker = true;
#endif
}

void ReadMWMTask::Reset()
{
#ifdef DEBUG
  m_checker = false;
#endif
}

void ReadMWMTask::Do()
{
#ifdef DEBUG
  ASSERT(m_checker, ());
#endif
  shared_ptr<TileInfo> tileInfo = m_tileInfo.lock();
  if (tileInfo == NULL)
    return;

  try
  {
    tileInfo->ReadFeatureIndex(m_model);
    tileInfo->ReadFeatures(m_model, m_memIndex);
  }
  catch (TileInfo::ReadCanceledException & ex)
  {
    return;
  }
}

TileKey ReadMWMTask::GetTileKey() const
{
  shared_ptr<TileInfo> tileInfo = m_tileInfo.lock();
  if (tileInfo == NULL)
    return TileKey();

  return tileInfo->GetTileKey();
}

} // namespace df
