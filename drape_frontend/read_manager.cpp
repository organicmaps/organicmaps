#include "drape_frontend/read_manager.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/platform.hpp"

#include "base/buffer_vector.hpp"
#include "base/stl_add.hpp"

#include "std/bind.hpp"
#include "std/algorithm.hpp"

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

ReadManager::ReadManager(ref_ptr<ThreadsCommutator> commutator, MapDataProvider & model)
  : m_commutator(commutator)
  , m_model(model)
  , myPool(64, ReadMWMTaskFactory(m_memIndex, m_model))
  , m_pool(make_unique_dp<threads::ThreadPool>(ReadCount(), bind(&ReadManager::OnTaskFinished, this, _1)))
  , m_counter(0)
{
}

void ReadManager::OnTaskFinished(threads::IRoutine * task)
{
  ASSERT(dynamic_cast<ReadMWMTask *>(task) != NULL, ());
  ReadMWMTask * t = static_cast<ReadMWMTask *>(task);

  // finish tiles
  {
    lock_guard<mutex> lock(m_finishedTilesMutex);

    // add finished tile to collection
    m_finishedTiles.emplace(t->GetTileKey());

    // decrement counter
    ASSERT(m_counter > 0, ());
    --m_counter;
    if (m_counter == 0)
    {
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<FinishReadingMessage>(m_finishedTiles),
                                MessagePriority::Normal);
      m_finishedTiles.clear();
    }
  }

  t->Reset();
  myPool.Return(t);
}

void ReadManager::UpdateCoverage(ScreenBase const & screen, TTilesCollection const & tiles)
{
  if (screen == m_currentViewport)
    return;

  if (MustDropAllTiles(screen))
  {
    IncreaseCounter(static_cast<int>(tiles.size()));

    for_each(m_tileInfos.begin(), m_tileInfos.end(), bind(&ReadManager::CancelTileInfo, this, _1));
    m_tileInfos.clear();
    for_each(tiles.begin(), tiles.end(), bind(&ReadManager::PushTaskBackForTileKey, this, _1));
  }
  else
  {
    // Find rects that go out from viewport
    buffer_vector<shared_ptr<TileInfo>, 8> outdatedTiles;
#ifdef _MSC_VER
    vs_bug::
#endif
    set_difference(m_tileInfos.begin(), m_tileInfos.end(),
                   tiles.begin(), tiles.end(),
                   back_inserter(outdatedTiles), LessCoverageCell());

    // Find rects that go in into viewport
    buffer_vector<TileKey, 8> inputRects;
#ifdef _MSC_VER
    vs_bug::
#endif
    set_difference(tiles.begin(), tiles.end(),
                   m_tileInfos.begin(), m_tileInfos.end(),
                   back_inserter(inputRects), LessCoverageCell());

    IncreaseCounter(static_cast<int>(inputRects.size() + (m_tileInfos.size() - outdatedTiles.size())));

    for_each(outdatedTiles.begin(), outdatedTiles.end(), bind(&ReadManager::ClearTileInfo, this, _1));
    for_each(m_tileInfos.begin(), m_tileInfos.end(), bind(&ReadManager::PushTaskFront, this, _1));
    for_each(inputRects.begin(), inputRects.end(), bind(&ReadManager::PushTaskBackForTileKey, this, _1));
  }
  m_currentViewport = screen;
}

void ReadManager::Invalidate(TTilesCollection const & keyStorage)
{
  for (auto & info : m_tileInfos)
  {
    if (keyStorage.find(info->GetTileKey()) != keyStorage.end())
    {
      CancelTileInfo(info);
      PushTaskFront(info);
    }
  }
}

void ReadManager::Stop()
{
  for_each(m_tileInfos.begin(), m_tileInfos.end(), bind(&ReadManager::CancelTileInfo, this, _1));
  m_tileInfos.clear();

  m_pool->Stop();
  m_pool.reset();
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
  shared_ptr<TileInfo> tileInfo(new TileInfo(EngineContext(tileKey, m_commutator)));
  m_tileInfos.insert(tileInfo);
  ReadMWMTask * task = myPool.Get();
  task->Init(tileInfo);
  m_pool->PushBack(task);
}

void ReadManager::PushTaskFront(shared_ptr<TileInfo> const & tileToReread)
{
  ReadMWMTask * task = myPool.Get();
  task->Init(tileToReread);
  m_pool->PushFront(task);
}

void ReadManager::CancelTileInfo(shared_ptr<TileInfo> const & tileToCancel)
{
  tileToCancel->Cancel(m_memIndex);
}

void ReadManager::ClearTileInfo(shared_ptr<TileInfo> const & tileToClear)
{
  CancelTileInfo(tileToClear);
  m_tileInfos.erase(tileToClear);
}

void ReadManager::IncreaseCounter(int value)
{
  lock_guard<mutex> lock(m_finishedTilesMutex);
  m_counter += value;
}

} // namespace df
