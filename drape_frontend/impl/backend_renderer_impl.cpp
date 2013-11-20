#include "backend_renderer_impl.hpp"

#include "threads_commutator.hpp"
#include "render_thread.hpp"
#include "tile_info.hpp"
#include "memory_feature_index.hpp"
#include "read_mwm_task.hpp"

#include "message.hpp"
#include "update_coverage_message.hpp"
#include "resize_message.hpp"
#include "task_finish_message.hpp"
#include "drop_tile_message.hpp"
#include "drop_coverage_message.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/buffer_vector.hpp"
#include "../base/object_tracker.hpp"

#include "../std/bind.hpp"

namespace
{
  void PostFinishTask(df::ThreadsCommutator * commutator, threads::IRoutine * routine)
  {
    commutator->PostMessage(df::ThreadsCommutator::ResourceUploadThread, new df::TaskFinishMessage(routine));
  }

  struct CoverageCellComparer
  {
    bool operator()(df::ReadMWMTask const * task, Tiler::RectInfo const & rectInfo) const
    {
      /// TODO remove RectInfo to TileInfo covertion after rewrite tiler on TileInfo returning
      return task->GetTileInfo() < df::TileInfo(rectInfo.m_x, rectInfo.m_y, rectInfo.m_tileScale);
    }

    bool operator()(Tiler::RectInfo const & rectInfo, df::ReadMWMTask const * task) const
    {
      /// TODO remove RectInfo to TileInfo covertion after rewrite tiler on TileInfo returning
      return df::TileInfo(rectInfo.m_x, rectInfo.m_y, rectInfo.m_tileScale) < task->GetTileInfo();
    }
  };
}

namespace df
{
  BackendRendererImpl::BackendRendererImpl(ThreadsCommutator * commutator,
                                           double visualScale,
                                           int surfaceWidth,
                                           int surfaceHeight)
    : m_commutator(commutator)
  {
    m_scaleProcessor.SetParams(visualScale, ScalesProcessor::CalculateTileSize(surfaceWidth, surfaceHeight));

    m_commutator->RegisterThread(ThreadsCommutator::ResourceUploadThread, this);
    m_pool = new threads::ThreadPool(max(1, GetPlatform().CpuCores() - 2), bind(&PostFinishTask, commutator, _1));

    /// TODO create batcher with thread thansfer interface
    StartThread();
  }

  BackendRendererImpl::~BackendRendererImpl()
  {
    StopThread();
    CloseQueue();

    m_pool->Stop();
    delete m_pool;

    typedef set<ReadMWMTask *>::iterator index_iter;
    for (index_iter it = m_taskIndex.begin(); it != m_taskIndex.end(); ++it)
      delete (*it);

    m_taskIndex.clear();
  }

  void BackendRendererImpl::UpdateCoverage(const ScreenBase & screen)
  {
    m_tiler.seed(screen, screen.GlobalRect().GlobalCenter(), m_scaleProcessor.GetTileSize());

    vector<Tiler::RectInfo> tiles;
    m_tiler.tiles(tiles, 1);

    if (!m_currentViewport.GlobalRect().IsIntersect(screen.GlobalRect()) ||
        m_scaleProcessor.GetTileScaleBase(m_currentViewport) != m_scaleProcessor.GetTileScaleBase(screen))
    {
      typedef set<ReadMWMTask *>::iterator index_iter;
      for (index_iter it = m_taskIndex.begin(); it != m_taskIndex.end(); ++it)
        CancelTask(*it, false, false);

      m_taskIndex.clear();
      PostToRenderThreads(new DropCoverageMessage());

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
        CancelTask(outdatedTasks[i], true, true);

      RestartExistTasks();

      for (size_t i = 0; i < inputRects.size(); ++i)
        CreateTask(inputRects[i]);
    }
  }

  void BackendRendererImpl::FinishTask(threads::IRoutine * routine)
  {
    ReadMWMTask * readTask = static_cast<ReadMWMTask *>(routine);
    readTask->Finish();
    if (routine->IsCancelled())
    {
      ASSERT(m_taskIndex.find(readTask) == m_taskIndex.end(), ());
      CancelTask(readTask, false, true);
    }
    else
    {
      /// TODO flush geometry backet on frontend renderer
    }
  }

  void BackendRendererImpl::Resize(m2::RectI const & rect)
  {
    m_currentViewport.OnSize(rect);
  }

  void BackendRendererImpl::CreateTask(Tiler::RectInfo const & info)
  {
    ReadMWMTask * task = new ReadMWMTask(TileKey(info.m_x, info.m_y, info.m_tileScale), m_index);
    m_taskIndex.insert(task);
    m_pool->AddTask(task);
  }

  void BackendRendererImpl::CancelTask(ReadMWMTask * task, bool removefromIndex, bool postToRenderer)
  {
    task->Cancel();

    if (postToRenderer)
      PostToRenderThreads(new DropTileMessage(task->GetTileInfo().m_key));

    if (removefromIndex)
      m_taskIndex.erase(task);

    m_index.RemoveFeatures(task->GetTileInfo().m_featureInfo);
    if (task->IsFinished())
      delete task;
  }

  void BackendRendererImpl::RestartExistTasks()
  {
    typedef set<ReadMWMTask *>::iterator index_iter;
    for (index_iter it = m_taskIndex.begin(); it != m_taskIndex.end(); ++it)
    {
      (*it)->PrepareToRestart();
      m_pool->AddTask(*it);
    }
  }

  /////////////////////////////////////////
  //           MessageAcceptor           //
  /////////////////////////////////////////
  void BackendRendererImpl::AcceptMessage(Message * message)
  {
    switch (message->GetType())
    {
    case Message::UpdateCoverage:
      UpdateCoverage(static_cast<UpdateCoverageMessage *>(message)->GetScreen());
      break;
    case Message::Resize:
      Resize(static_cast<ResizeMessage *>(message)->GetRect());
      break;
    case Message::TaskFinish:
      FinishTask(static_cast<TaskFinishMessage *>(message)->GetRoutine());
      break;
    default:
      ASSERT(false, ());
      break;
    }
  }

  /////////////////////////////////////////
  //             ThreadPart              //
  /////////////////////////////////////////
  void BackendRendererImpl::StartThread()
  {
    m_selfThread.Create(this);
  }

  void BackendRendererImpl::StopThread()
  {
    m_selfThread.Cancel();
  }

  void BackendRendererImpl::ThreadMain()
  {
    InitRenderThread();

    while (!IsCancelled())
      ProcessSingleMessage(true);
  }

  void BackendRendererImpl::Do()
  {
    ThreadMain();
  }

  void BackendRendererImpl::PostToRenderThreads(Message * message)
  {
    m_commutator->PostMessage(ThreadsCommutator::RenderThread, message);
  }
}
