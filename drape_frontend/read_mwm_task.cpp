#include "read_mwm_task.hpp"

#include "../std/shared_ptr.hpp"

namespace df
{

  ReadMWMTask::ReadMWMTask(weak_ptr<TileInfo> const & tileInfo, EngineContext & context)
    : m_tileInfo(tileInfo)
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
      tileInfo->ReadFeatureIndex();
      tileInfo->ReadFeatures(m_context);
    }
    catch (TileInfo::ReadCanceledException & ex)
    {
      return;
    }
  }
}
