#pragma once

#include "tile_info.hpp"
#include "memory_feature_index.hpp"
#include "engine_context.hpp"

#include "../base/thread.hpp"
#include "../base/object_tracker.hpp"

namespace df
{
  class ReadMWMTask : public threads::IRoutine
  {
  public:
    ReadMWMTask(TileKey const & tileKey,
                MemoryFeatureIndex & index,
                EngineContext & context);

    virtual void Do();

    df::TileInfo const & GetTileInfo() const;

    void PrepareToRestart();
    void Finish();
    bool IsFinished();

  private:
    void ReadTileIndex();
    void ReadGeometry(const FeatureID & id);

  private:
    TileInfo m_tileInfo;
    bool m_isFinished;
    MemoryFeatureIndex & m_index;
    EngineContext & m_context;

  #ifdef DEBUG
    dbg::ObjectTracker m_objTracker;
  #endif
  };

  struct ReadMWMTaskLess
  {
    bool operator()( const ReadMWMTask * l, const ReadMWMTask * r ) const
    {
      return l->GetTileInfo().m_key < r->GetTileInfo().m_key;
    }
  };
}
