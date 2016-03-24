#pragma once

#include "drape_frontend/engine_context.hpp"
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
  ReadManager(ref_ptr<ThreadsCommutator> commutator, MapDataProvider & model, bool allow3dBuildings);

  void UpdateCoverage(ScreenBase const & screen, bool have3dBuildings,
                      TTilesCollection const & tiles, ref_ptr<dp::TextureManager> texMng);
  void Invalidate(TTilesCollection const & keyStorage);
  void InvalidateAll();
  void Stop();

  bool CheckTileKey(TileKey const & tileKey) const;
  void Allow3dBuildings(bool allow3dBuildings);

  static size_t ReadCount();

private:
  void OnTaskFinished(threads::IRoutine * task);
  bool MustDropAllTiles(ScreenBase const & screen) const;

  void PushTaskBackForTileKey(TileKey const & tileKey, ref_ptr<dp::TextureManager> texMng);

private:
  ref_ptr<ThreadsCommutator> m_commutator;

  MapDataProvider & m_model;

  drape_ptr<threads::ThreadPool> m_pool;

  ScreenBase m_currentViewport;
  bool m_have3dBuildings;
  bool m_allow3dBuildings;
  bool m_modeChanged;

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
  mutex m_finishedTilesMutex;
  uint64_t m_generationCounter;

  using TTileInfoCollection = buffer_vector<shared_ptr<TileInfo>, 8>;
  TTilesCollection m_activeTiles;

  void CancelTileInfo(shared_ptr<TileInfo> const & tileToCancel);
  void ClearTileInfo(shared_ptr<TileInfo> const & tileToClear);
  void IncreaseCounter(int value);
  void CheckFinishedTiles(TTileInfoCollection const & requestedTiles);
};

} // namespace df
