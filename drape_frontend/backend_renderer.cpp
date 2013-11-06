#include "backend_renderer.hpp"

#include "render_thread.hpp"
#include "tile_info.hpp"
#include "memory_feature_index.hpp"

#include "../map/tiler.hpp"
#include "../map/scales_processor.hpp"

#include "../geometry/screenbase.hpp"

#include "../drape/batcher.hpp"

#include "../base/thread.hpp"
#include "../base/thread_pool.hpp"
#include "../base/buffer_vector.hpp"
#include "../base/object_tracker.hpp"

#include "../std/bind.hpp"
#include "../std/set.hpp"

namespace
{
  class ReadMWMTask : public threads::IRoutine
  {
  public:
    ReadMWMTask(Tiler::RectInfo const & tileId, df::MemoryFeatureIndex & index)
      : m_tileInfo(tileId.m_x, tileId.m_y, tileId.m_tileScale)
      , m_isFinished(false)
      , m_index(index)
    {
    }

    df::TileInfo const & GetTileInfo() const
    {
      return m_tileInfo;
    }

    virtual void Do()
    {
      if (m_tileInfo.m_featureInfo.empty())
        ReadTileIndex();

      vector<size_t> indexesToRead;
      m_index.ReadFeaturesRequest(m_tileInfo.m_featureInfo, indexesToRead);
      for (size_t i = 0; i < indexesToRead.size(); ++i)
      {
        df::FeatureInfo & info = m_tileInfo.m_featureInfo[i];
        ReadGeometry(info.m_id);
        info.m_isOwner = true;
      }
    }

    void PrepareToRestart()
    {
      m_isFinished = false;
    }

    void Finish()
    {
      m_isFinished = true;
    }

    bool IsFinished()
    {
      return m_isFinished;
    }

  private:
    void ReadTileIndex()
    {
      /// TODO read index specified by m_tileInfo(m_x & m_y & m_zoomLevel)
      /// TODO insert readed FeatureIDs into m_tileInfo.m_featureInfo;
    }

    void ReadGeometry(const FeatureID & id)
    {
      ///TODO read geometry
    }

  private:
    df::TileInfo m_tileInfo;
    bool m_isFinished;
    df::MemoryFeatureIndex & m_index;

#ifdef DEBUG
    dbg::ObjectTracker m_objTracker;
#endif
  };

  struct CoverageCellComparer
  {
    bool operator()(ReadMWMTask const * task, Tiler::RectInfo const & rectInfo) const
    {
      /// TODO remove RectInfo to TileInfo covertion after rewrite tiler on TileInfo returning
      return task->GetTileInfo() < df::TileInfo(rectInfo.m_x, rectInfo.m_y, rectInfo.m_tileScale);
    }

    bool operator()(Tiler::RectInfo const & rectInfo, ReadMWMTask const * task) const
    {
      /// TODO remove RectInfo to TileInfo covertion after rewrite tiler on TileInfo returning
      return df::TileInfo(rectInfo.m_x, rectInfo.m_y, rectInfo.m_tileScale) < task->GetTileInfo();
    }
  };
}

namespace df
{
  class BackendRenderer::Impl
  {
  public:
    Impl(threads::ThreadPool * pool, size_t tileSize)
      : m_pool(pool)
      , m_scaleProcessor(tileSize)
    {
      /// TODO create batcher with thread thansfer interface
    }

    ~Impl()
    {
      m_pool->Stop();
      delete m_pool;

      typedef set<ReadMWMTask *>::iterator index_iter;
      for (index_iter it = m_taskIndex.begin(); it != m_taskIndex.end(); ++it)
        delete (*it);

      m_taskIndex.clear();
    }

    void UpdateCoverage(const ScreenBase & screen)
    {
      m_tiler.seed(screen, screen.GlobalRect().GlobalCenter(), m_scaleProcessor.GetTileSize());

      vector<Tiler::RectInfo> tiles;
      m_tiler.tiles(tiles, 1);

      if (!m_currentViewport.GlobalRect().IsIntersect(screen.GlobalRect()) ||
          m_scaleProcessor.GetTileScaleBase(m_currentViewport) != m_scaleProcessor.GetTileScaleBase(screen))
      {
        typedef set<ReadMWMTask *>::iterator index_iter;
        for (index_iter it = m_taskIndex.begin(); it != m_taskIndex.end(); ++it)
          CancelTask(*it, false);

        m_taskIndex.clear();

        for (size_t i = 0; i < tiles.size(); ++i)
          CreateTask(tiles[i]);
      }
      else
      {
        set<Tiler::RectInfo> rectInfoSet;
        for (size_t i = 0 ; i < tiles.size(); ++i)
          rectInfoSet.insert(tiles[i]);

        // Find rects that go out from viewport
        buffer_vector<ReadMWMTask *, 8> outdatedTasks;
        set_difference(m_taskIndex.begin(), m_taskIndex.end(),
                       rectInfoSet.begin(), rectInfoSet.end(),
                       back_inserter(outdatedTasks), CoverageCellComparer());

        // Find rects that go in into viewport
        buffer_vector<Tiler::RectInfo, 8> inputRects;
        set_difference(rectInfoSet.begin(), rectInfoSet.end(),
                       m_taskIndex.begin(), m_taskIndex.end(),
                       back_inserter(inputRects), CoverageCellComparer());

        for (size_t i = 0; i < outdatedTasks.size(); ++i)
          CancelTask(outdatedTasks[i], true);

        RestartExistTasks();

        for (size_t i = 0; i < inputRects.size(); ++i)
          CreateTask(inputRects[i]);
      }
    }

    void Resize(m2::RectI const & rect)
    {
      m_currentViewport.OnSize(rect);
    }

    void FinishTask(threads::IRoutine * routine)
    {
      ReadMWMTask * readTask = static_cast<ReadMWMTask *>(routine);
      readTask->Finish();
      if (routine->IsCancelled())
      {
        ASSERT(m_taskIndex.find(readTask) == m_taskIndex.end(), ());

        /// TODO remove geometry from render loop
        /// TODO remove unflushed backets
        m_index.RemoveFeatures(readTask->GetTileInfo().m_featureInfo);

        delete readTask;
      }
      else
      {
        /// TODO flush geometry backet on frontend renderer
      }
    }

  private:
    void CreateTask(Tiler::RectInfo const & info)
    {
      ReadMWMTask * task = new ReadMWMTask(info, m_index);
      m_taskIndex.insert(task);
      m_pool->AddTask(task);
    }

    void CancelTask(ReadMWMTask * task, bool removeFromIndex)
    {
      task->Cancel();
      if (removeFromIndex)
        m_taskIndex.erase(task);
      m_index.RemoveFeatures(task->GetTileInfo().m_featureInfo);
      if (task->IsFinished())
        delete task;
    }

    void RestartExistTasks()
    {
      typedef set<ReadMWMTask *>::iterator index_iter;
      for (index_iter it = m_taskIndex.begin(); it != m_taskIndex.end(); ++it)
      {
        (*it)->PrepareToRestart();
        m_pool->AddTask(*it);
      }
    }

  private:
    threads::ThreadPool * m_pool;

    ScreenBase m_currentViewport;
    set<ReadMWMTask *> m_taskIndex;

    Tiler m_tiler;
    ScalesProcessor m_scaleProcessor;

    MemoryFeatureIndex m_index;

    //Batcher * m_batcher;
  };

  BackendRenderer::BackendRenderer(int cpuCoreCount, size_t tileSize)
    : m_queue(1)
  {
    m_impl = new Impl(new threads::ThreadPool(cpuCoreCount, bind(&BackendRenderer::FinishTask, this, _1)), tileSize);
    m_queue.AddInitCommand(bind(&InitRenderThread));
  }

  BackendRenderer::~BackendRenderer()
  {
    m_queue.Cancel();
    delete m_impl;
  }

  void BackendRenderer::UpdateCoverage(const ScreenBase & screen)
  {
    m_queue.AddCommand(bind(&BackendRenderer::Impl::UpdateCoverage, m_impl, screen));
  }

  void BackendRenderer::Resize(int x0, int y0, int w, int h)
  {
    m_queue.AddCommand(bind(&BackendRenderer::Impl::Resize, m_impl, m2::RectI(x0, y0, x0 + w, y0 + h)));
  }

  void BackendRenderer::FinishTask(threads::IRoutine * routine)
  {
    m_queue.AddCommand(bind(&BackendRenderer::Impl::FinishTask, m_impl, routine));
  }
}
