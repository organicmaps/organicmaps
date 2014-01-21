#pragma once

#include "tile_info.hpp"
#include "engine_context.hpp"

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
    ReadMWMTask(weak_ptr<TileInfo> const & tileInfo, EngineContext & context);

    virtual void Do();

  private:
    weak_ptr<TileInfo> m_tileInfo;
    EngineContext & m_context;
  #ifdef DEBUG
    dbg::ObjectTracker m_objTracker;
  #endif
  };
}
