#pragma once

#include "memory_feature_index.hpp"
#include "engine_context.hpp"
#include "tile_info.hpp"

#include "../drape/pointers.hpp"

#include "../map/feature_vec_model.hpp"
#include "../map/scales_processor.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/thread_pool.hpp"

#include "../std/set.hpp"
#include "../std/shared_ptr.hpp"


namespace df
{
  class CoverageUpdateDescriptor;

  typedef shared_ptr<TileInfo> tileinfo_ptr;

  class ReadManager
  {
  public:
    ReadManager(EngineContext & context, model::FeaturesFetcher & model);

    void UpdateCoverage(ScreenBase const & screen, CoverageUpdateDescriptor & updateDescr);
    void Stop();

    static size_t ReadCount();

  private:
    void OnTaskFinished(threads::IRoutine * task);
    void GetTileKeys(set<TileKey> & out, const ScreenBase & screen) const;
    bool MustDropAllTiles(ScreenBase const & screen) const;

    void PushTaskBackForTileKey(TileKey const & tileKey);
    void PushTaskFront(tileinfo_ptr const & tileToReread);

  private:
    MemoryFeatureIndex m_memIndex;
    EngineContext & m_context;

    model::FeaturesFetcher & m_model;

    MasterPointer<threads::ThreadPool> m_pool;

    ScreenBase m_currentViewport;

    struct LessByTileKey
    {
      bool operator ()(tileinfo_ptr const & l, tileinfo_ptr const & r) const
      {
        return *l < *r;
      }
    };

    set<tileinfo_ptr, LessByTileKey> m_tileInfos;

    void CancelTileInfo(tileinfo_ptr tileToCancel);
    void ClearTileInfo(tileinfo_ptr & tileToClear);
  };
}
