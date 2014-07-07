#include "read_manager.hpp"
#include "read_mwm_task.hpp"
#include "visual_params.hpp"

#include "../platform/platform.hpp"

#include "../base/buffer_vector.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"
#include "../std/algorithm.hpp"

namespace df
{

namespace
{

struct LessCoverageCell
{
  bool operator()(shared_ptr<TileInfo> const & l, TileKey const & r) const
  {
    return l->GetTileKey() < r;
  }

  bool operator()(TileKey const & l, shared_ptr<TileInfo> const & r) const
  {
    return l < r->GetTileKey();
  }
};

} // namespace

ReadManager::ReadManager(EngineContext & context, model::FeaturesFetcher & model)
  : m_context(context)
  , m_model(model)
{
  m_pool.Reset(new threads::ThreadPool(ReadCount(), bind(&ReadManager::OnTaskFinished, this, _1)));
}

void ReadManager::OnTaskFinished(threads::IRoutine * task)
{
  delete task;
}

void ReadManager::UpdateCoverage(ScreenBase const & screen, set<TileKey> const & tiles)
{
  if (screen == m_currentViewport)
    return;

  if (MustDropAllTiles(screen))
  {
    for_each(m_tileInfos.begin(), m_tileInfos.end(), bind(&ReadManager::CancelTileInfo, this, _1));
    m_tileInfos.clear();

    for_each(tiles.begin(), tiles.end(), bind(&ReadManager::PushTaskBackForTileKey, this, _1));
  }
  else
  {
    // Find rects that go out from viewport
    buffer_vector<tileinfo_ptr, 8> outdatedTiles;
    set_difference(m_tileInfos.begin(), m_tileInfos.end(),
                   tiles.begin(), tiles.end(),
                   back_inserter(outdatedTiles), LessCoverageCell());

    // Find rects that go in into viewport
    buffer_vector<TileKey, 8> inputRects;
    set_difference(tiles.begin(), tiles.end(),
                   m_tileInfos.begin(), m_tileInfos.end(),
                   back_inserter(inputRects), LessCoverageCell());

    for_each(outdatedTiles.begin(), outdatedTiles.end(), bind(&ReadManager::ClearTileInfo, this, _1));
    for_each(m_tileInfos.begin(), m_tileInfos.end(), bind(&ReadManager::PushTaskFront, this, _1));
    for_each(inputRects.begin(),  inputRects.end(),  bind(&ReadManager::PushTaskBackForTileKey, this, _1));
  }
  m_currentViewport = screen;
}

void ReadManager::Invalidate(set<TileKey> const & keyStorage)
{
  tile_set_t::iterator it = m_tileInfos.begin();
  for (; it != m_tileInfos.end(); ++it)
  {
    if (keyStorage.find((*it)->GetTileKey()) != keyStorage.end())
    {
      CancelTileInfo(*it);
      PushTaskFront(*it);
    }
  }
}

void ReadManager::Stop()
{
  for_each(m_tileInfos.begin(), m_tileInfos.end(), bind(&ReadManager::CancelTileInfo, this, _1));
  m_tileInfos.clear();

  m_pool->Stop();
  m_pool.Destroy();
}

size_t ReadManager::ReadCount()
{
  return max(GetPlatform().CpuCores() - 2, 1);
}

bool ReadManager::MustDropAllTiles(ScreenBase const & screen) const
{
  int const oldScale = df::GetTileScaleBase(m_currentViewport);
  int const newScale = df::GetTileScaleBase(screen);
  return (oldScale != newScale) || !m_currentViewport.GlobalRect().IsIntersect(screen.GlobalRect());
}

void ReadManager::PushTaskBackForTileKey(TileKey const & tileKey)
{
  tileinfo_ptr tileInfo(new TileInfo(tileKey));
  m_tileInfos.insert(tileInfo);
  m_pool->PushBack(new ReadMWMTask(tileInfo, m_memIndex, m_model, m_context));
}

void ReadManager::PushTaskFront(tileinfo_ptr const & tileToReread)
{
  m_pool->PushFront(new ReadMWMTask(tileToReread, m_memIndex, m_model, m_context));
}

void ReadManager::CancelTileInfo(tileinfo_ptr const & tileToCancel)
{
  tileToCancel->Cancel(m_memIndex);
}

void ReadManager::ClearTileInfo(tileinfo_ptr const & tileToClear)
{
  CancelTileInfo(tileToClear);
  m_tileInfos.erase(tileToClear);
}

} // namespace df

