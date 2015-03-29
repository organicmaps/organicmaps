#pragma once

#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/memory_feature_index.hpp"
#include "drape_frontend/read_mwm_task.hpp"
#include "drape_frontend/tile_info.hpp"

#include "geometry/screenbase.hpp"

#include "drape/object_pool.hpp"
#include "drape/pointers.hpp"

#include "base/thread_pool.hpp"

#include "std/set.hpp"
#include "std/shared_ptr.hpp"

namespace df
{

class MapDataProvider;
class CoverageUpdateDescriptor;

typedef shared_ptr<TileInfo> TTileInfoPtr;

class ReadManager
{
public:
  ReadManager(dp::RefPointer<ThreadsCommutator> commutator, MapDataProvider & model);

  void UpdateCoverage(ScreenBase const & screen, set<TileKey> const & tiles);
  void Invalidate(set<TileKey> const & keyStorage);
  void Stop();

  static size_t ReadCount();

private:
  void OnTaskFinished(threads::IRoutine * task);
  bool MustDropAllTiles(ScreenBase const & screen) const;

  void PushTaskBackForTileKey(TileKey const & tileKey);
  void PushTaskFront(TTileInfoPtr const & tileToReread);

private:
  MemoryFeatureIndex m_memIndex;
  dp::RefPointer<ThreadsCommutator> m_commutator;

  MapDataProvider & m_model;

  dp::MasterPointer<threads::ThreadPool> m_pool;

  ScreenBase m_currentViewport;

  struct LessByTileKey
  {
    bool operator ()(TTileInfoPtr const & l, TTileInfoPtr const & r) const
    {
      return *l < *r;
    }
  };

  using TTileSet = set<TTileInfoPtr, LessByTileKey>;
  TTileSet m_tileInfos;

  ObjectPool<ReadMWMTask, ReadMWMTaskFactory> myPool;

  void CancelTileInfo(TTileInfoPtr const & tileToCancel);
  void ClearTileInfo(TTileInfoPtr const & tileToClear);
};

} // namespace df
