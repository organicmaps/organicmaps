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

void ReadMWMTask::Do()
{
#ifdef DEBUG
  ASSERT(m_checker, ());
#endif

  ASSERT(m_tileInfo != nullptr, ());
  try
  {
    m_tileInfo->ReadFeatures(m_model, m_memIndex);
  }
  catch (TileInfo::ReadCanceledException &)
  {
    return;
  }
}

TileKey ReadMWMTask::GetTileKey() const
{
  ASSERT(m_tileInfo != nullptr, ());
  return m_tileInfo->GetTileKey();
}

} // namespace df
