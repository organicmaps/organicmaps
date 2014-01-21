#include "read_manager.hpp"
#include "read_mwm_task.hpp"
#include "coverage_update_descriptor.hpp"

#include "../platform/platform.hpp"

#include "../base/buffer_vector.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"
#include "../std/algorithm.hpp"

namespace df
{

namespace
{

void CancelTaskFn(shared_ptr<TileInfo> tinfo)
{
  tinfo->Cancel();
}

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

TileKey TileInfoPtrToTileKey(shared_ptr<TileInfo> const & p)
{
  return p->GetTileKey();
}

}

ReadManager::ReadManager(double visualScale, int w, int h,
                             EngineContext & context,
                             model::FeaturesFetcher & model)
  : m_context(context)
  , m_model(model)
{
  m_scalesProcessor.SetParams(visualScale, ScalesProcessor::CalculateTileSize(w, h));
  m_pool.Reset(new threads::ThreadPool(ReadCount(), bind(&ReadManager::OnTaskFinished, this, _1)));
}

void ReadManager::OnTaskFinished(threads::IRoutine * task)
{
  delete task;
}

void ReadManager::UpdateCoverage(const ScreenBase & screen, CoverageUpdateDescriptor & updateDescr)
{
  if (screen == m_currentViewport)
    return;

  set<TileKey> tiles;
  GetTileKeys(tiles, screen);

  if (MustDropAllTiles(screen))
  {
    for_each(m_tileInfos.begin(), m_tileInfos.end(), &CancelTaskFn);
    m_tileInfos.clear();

    for_each(tiles.begin(), tiles.end(), bind(&ReadManager::PushTaskBackForTileKey, this, _1));

    updateDescr.DoDropAll();
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

    buffer_vector<TileKey, 16> outdatedTileKeys;
    transform(outdatedTiles.begin(), outdatedTiles.end(),
              back_inserter(outdatedTileKeys), &TileInfoPtrToTileKey);

    updateDescr.DropTiles(outdatedTileKeys.data(), outdatedTileKeys.size());

    for_each(m_tileInfos.begin(), m_tileInfos.end(), bind(&ReadManager::PushTaskFront, this, _1));
    for_each(inputRects.begin(),  inputRects.end(),  bind(&ReadManager::PushTaskBackForTileKey, this, _1));
  }
  m_currentViewport = screen;
}

void ReadManager::Resize(const m2::RectI & rect)
{
  m_currentViewport.OnSize(rect);
}

void ReadManager::Stop()
{
  for_each(m_tileInfos.begin(), m_tileInfos.end(), &CancelTaskFn);
  m_tileInfos.clear();

  m_pool->Stop();
  m_pool.Destroy();
}

size_t ReadManager::ReadCount()
{
  return max(GetPlatform().CpuCores() - 2, 1);
}

void ReadManager::GetTileKeys(set<TileKey> & out, ScreenBase const & screen) const
{
  out.clear();

  int const tileScale = m_scalesProcessor.GetTileScaleBase(screen);
  // equal for x and y
  double const range = MercatorBounds::maxX - MercatorBounds::minX;
  double const rectSize = range / (1 << tileScale);

  m2::AnyRectD const & globalRect = screen.GlobalRect();
  m2::RectD    const & clipRect   = globalRect.GetGlobalRect();

  int const minTileX = static_cast<int>(floor(clipRect.minX() / rectSize));
  int const maxTileX = static_cast<int>(ceil(clipRect.maxX() / rectSize));
  int const minTileY = static_cast<int>(floor(clipRect.minY() / rectSize));
  int const maxTileY = static_cast<int>(ceil(clipRect.maxY() / rectSize));

  for (int tileY = minTileY; tileY < maxTileY; ++tileY)
    for (int tileX = minTileX; tileX < maxTileX; ++tileX)
    {
      double const left = tileX * rectSize;
      double const top  = tileY * rectSize;

      m2::RectD currentTileRect(left, top,
                                left + rectSize, top + rectSize);

      if (globalRect.IsIntersect(m2::AnyRectD(currentTileRect)))
        out.insert(TileKey(tileX, tileY, tileScale));
    }
}

bool ReadManager::MustDropAllTiles(ScreenBase const & screen) const
{
  const int oldScale = m_scalesProcessor.GetTileScaleBase(m_currentViewport);
  const int newScale = m_scalesProcessor.GetTileScaleBase(screen);
  return (oldScale != newScale) || !m_currentViewport.GlobalRect().IsIntersect(screen.GlobalRect());
}

void ReadManager::PushTaskBackForTileKey(TileKey const & tileKey)
{
  tileinfo_ptr tileInfo(new TileInfo(tileKey, m_model, m_memIndex));
  m_tileInfos.insert(tileInfo);
  m_pool->PushBack(new ReadMWMTask(tileInfo, m_context));
}

void ReadManager::PushTaskFront(ReadManager::tileinfo_ptr const & tileToReread)
{
  m_pool->PushFront(new ReadMWMTask(tileToReread, m_context));
}

void ReadManager::ClearTileInfo(ReadManager::tileinfo_ptr & tileToClear)
{
  tileToClear->Cancel();
  m_tileInfos.erase(tileToClear);
}

}

