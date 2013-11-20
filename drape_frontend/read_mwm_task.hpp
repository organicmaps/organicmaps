#pragma once

#include "tile_info.hpp"
#include "memory_feature_index.hpp"

#include "../base/thread.hpp"
#include "../base/object_tracker.hpp"

namespace df
{
  class ReadMWMTask : public threads::IRoutine
  {
  public:
    ReadMWMTask(TileKey const & tileKey, df::MemoryFeatureIndex & index);

    virtual void Do();

    df::TileInfo const & GetTileInfo() const;

    void PrepareToRestart();
    void Finish();
    bool IsFinished();

  private:
    void ReadTileIndex();
    void ReadGeometry(const FeatureID & id);

  private:
    df::TileInfo m_tileInfo;
    bool m_isFinished;
    df::MemoryFeatureIndex & m_index;

  #ifdef DEBUG
    dbg::ObjectTracker m_objTracker;
  #endif
  };
}
