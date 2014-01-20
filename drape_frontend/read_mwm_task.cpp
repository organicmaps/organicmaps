#include "read_mwm_task.hpp"

#include "../std/shared_ptr.hpp"

namespace df
{

  ReadMWMTask::ReadMWMTask(weak_ptr<TileInfo> const & tileInfo)
    : m_tileInfo(tileInfo)
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
      tileInfo->ReadFeatures();
    }
    catch (TileInfo::ReadCanceledException & ex)
    {
      return;
    }
  }
}
