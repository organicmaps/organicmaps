#pragma once

#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/memory_feature_index.hpp"
#include "drape_frontend/read_mwm_task.hpp"
#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "geometry/screenbase.hpp"

#include "drape/object_pool.hpp"
#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "base/thread_pool.hpp"

#include "std/atomic.hpp"
#include "std/mutex.hpp"
#include "std/set.hpp"
#include "std/shared_ptr.hpp"

namespace df
{

class MapDataProvider;
class CoverageUpdateDescriptor;

class ReadManager
{
public:
  ReadManager(ref_ptr<ThreadsCommutator> commutator, MapDataProvider & model);

  void UpdateCoverage(ScreenBase const & screen, TTilesCollection const & tiles, ref_ptr<dp::TextureManager> texMng);
  void Invalidate(TTilesCollection const & keyStorage);
  void Stop();

  static size_t ReadCount();

private:
  void OnTaskFinished(threads::IRoutine * task);
  bool MustDropAllTiles(ScreenBase const & screen) const;

  void PushTaskBackForTileKey(TileKey const & tileKey, ref_ptr<dp::TextureManager> texMng);
  void PushTaskFront(shared_ptr<TileInfo> const & tileToReread, ref_ptr<dp::TextureManager> texMng);

private:
  MemoryFeatureIndex m_memIndex;
  ref_ptr<ThreadsCommutator> m_commutator;

  MapDataProvider & m_model;

  drape_ptr<threads::ThreadPool> m_pool;

  ScreenBase m_currentViewport;
  bool m_forceUpdate;

  struct LessByTileInfo
  {
    bool operator ()(shared_ptr<TileInfo> const & l, shared_ptr<TileInfo> const & r) const
    {
      return *l < *r;
    }
  };

  using TTileSet = set<shared_ptr<TileInfo>, LessByTileInfo>;
  TTileSet m_tileInfos;

  ObjectPool<ReadMWMTask, ReadMWMTaskFactory> myPool;

  int m_counter;
  set<TileKey> m_finishedTiles;
  mutex m_finishedTilesMutex;

  void CancelTileInfo(shared_ptr<TileInfo> const & tileToCancel);
  void ClearTileInfo(shared_ptr<TileInfo> const & tileToClear);
  void IncreaseCounter(int value);
};

} // namespace df
