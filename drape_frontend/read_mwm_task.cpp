#include "read_mwm_task.hpp"

#include "../std/shared_ptr.hpp"

namespace df
{

ReadMWMTask::ReadMWMTask(weak_ptr<TileInfo> const & tileInfo,
                         MemoryFeatureIndex & memIndex,
                         model::FeaturesFetcher & model,
                         EngineContext & context)
  : m_tileInfo(tileInfo)
  , m_memIndex(memIndex)
  , m_model(model)
  , m_context(context)
{
}

void ReadMWMTask::Do()
{
  shared_ptr<TileInfo> tileInfo = m_tileInfo.lock();
  if (tileInfo == NULL)
    return;

  try
  {
    tileInfo->ReadFeatureIndex(m_model);
    tileInfo->ReadFeatures(m_model, m_memIndex, m_context);
  }
  catch (TileInfo::ReadCanceledException & ex)
  {
    return;
  }
}

} // namespace df
