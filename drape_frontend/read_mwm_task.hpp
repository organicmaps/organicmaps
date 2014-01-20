#pragma once

#include "tile_info.hpp"

#include "../base/thread.hpp"

#ifdef DEBUG
  #include "../base/object_tracker.hpp"
#endif

#include "../std/weak_ptr.hpp"

namespace df
{
  class ReadMWMTask : public threads::IRoutine
  {
  public:
    ReadMWMTask(weak_ptr<TileInfo> const & tileInfo);

    virtual void Do();

  private:
    weak_ptr<TileInfo> m_tileInfo;

  #ifdef DEBUG
    dbg::ObjectTracker m_objTracker;
  #endif
  };
}
