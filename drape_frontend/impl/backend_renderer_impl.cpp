#include "backend_renderer_impl.hpp"

#include "threads_commutator.hpp"
#include "tile_info.hpp"
#include "memory_feature_index.hpp"
#include "read_mwm_task.hpp"

#include "message.hpp"
#include "message_subclasses.hpp"
#include "map_shape.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/buffer_vector.hpp"
#include "../base/object_tracker.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"

namespace
{
  void PostFinishTask(RefPointer<df::ThreadsCommutator> commutator, threads::IRoutine * routine)
  {
    commutator->PostMessage(df::ThreadsCommutator::ResourceUploadThread, MovePointer<df::Message>(new df::TaskFinishMessage(routine)));
  }

  struct CoverageCellComparer
  {
    bool operator()(df::ReadMWMTask const * task, df::TileKey const & tileKey) const
    {
      return task->GetTileInfo().m_key < tileKey;
    }

    bool operator()(df::TileKey const & tileKey, df::ReadMWMTask const * task) const
    {
      return tileKey < task->GetTileInfo().m_key;
    }
  };
}

namespace df
{
  BackendRendererImpl::BackendRendererImpl(RefPointer<ThreadsCommutator> commutator
                                           ,RefPointer<OGLContextFactory> oglcontextfactory
                                           ,double visualScale
                                           ,int surfaceWidth
                                           ,int surfaceHeight)
    : m_engineContext(commutator)
    , m_commutator(commutator)
    , m_contextFactory(oglcontextfactory)
  {
    m_scaleProcessor.SetParams(visualScale, ScalesProcessor::CalculateTileSize(surfaceWidth, surfaceHeight));
    m_currentViewport.SetFromRect(m2::AnyRectD(m_scaleProcessor.GetWorldRect()));

    m_commutator->RegisterThread(ThreadsCommutator::ResourceUploadThread, this);

    int readerCount = 1;//max(1, GetPlatform().CpuCores() - 2);
    m_threadPool.Reset(new threads::ThreadPool(readerCount, bind(&PostFinishTask, commutator, _1)));
    m_batchersPool.Reset(new BatchersPool(readerCount, bind(&BackendRendererImpl::PostToRenderThreads, this, _1)));

    StartThread();
  }

  BackendRendererImpl::~BackendRendererImpl()
  {
    StopThread();
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
      PostToRenderThreads(MovePointer<Message>(new DropCoverageMessage()));

      for (size_t i = 0; i < tiles.size(); ++i)
      {
        const Tiler::RectInfo & info = tiles[i];
        CreateTask(TileKey(info.m_x, info.m_y, info.m_tileScale));
      }
    }
    else
    {
      set<TileKey> rectInfoSet;
      for (size_t i = 0 ; i < tiles.size(); ++i)
        rectInfoSet.insert(TileKey(tiles[i].m_x, tiles[i].m_y, tiles[i].m_tileScale));

      // Find rects that go out from viewport
      buffer_vector<ReadMWMTask *, 8> outdatedTasks;
      set_difference(m_taskIndex.begin(), m_taskIndex.end(),
                     rectInfoSet.begin(), rectInfoSet.end(),
                     back_inserter(outdatedTasks), CoverageCellComparer());

      // Find rects that go in into viewport
      buffer_vector<TileKey, 8> inputRects;
      set_difference(rectInfoSet.begin(), rectInfoSet.end(),
                     m_taskIndex.begin(), m_taskIndex.end(),
                     back_inserter(inputRects), CoverageCellComparer());

      for (size_t i = 0; i < outdatedTasks.size(); ++i)
        CancelTask(outdatedTasks[i], true, true);

      RestartExistTasks();

      for (size_t i = 0; i < inputRects.size(); ++i)
        CreateTask(inputRects[i]);
    }

    m_currentViewport = screen;
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
  }

  void BackendRendererImpl::Resize(m2::RectI const & rect)
  {
    m_currentViewport.OnSize(rect);
    UpdateCoverage(m_currentViewport);
  }

  void BackendRendererImpl::CreateTask(TileKey const & info)
  {
    ReadMWMTask * task = new ReadMWMTask(info, m_index, m_engineContext);
    m_taskIndex.insert(task);
    m_threadPool->AddTask(task);
  }

  void BackendRendererImpl::CancelTask(ReadMWMTask * task, bool removefromIndex, bool postToRenderer)
  {
    task->Cancel();

    if (postToRenderer)
      PostToRenderThreads(MovePointer<Message>(new DropTileMessage(task->GetTileInfo().m_key)));

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
      m_threadPool->AddTask(*it);
    }
  }

  /////////////////////////////////////////
  //           MessageAcceptor           //
  /////////////////////////////////////////
  void BackendRendererImpl::AcceptMessage(RefPointer<Message> message)
  {
    switch (message->GetType())
    {
    case Message::UpdateCoverage:
      UpdateCoverage(static_cast<UpdateCoverageMessage *>(message.GetRaw())->GetScreen());
      break;
    case Message::Resize:
      Resize(static_cast<ResizeMessage *>(message.GetRaw())->GetRect());
      break;
    case Message::TaskFinish:
      FinishTask(static_cast<TaskFinishMessage *>(message.GetRaw())->GetRoutine());
      break;
    case Message::TileReadStarted:
    case Message::TileReadEnded:
    case Message::MapShapeReaded:
      m_batchersPool->AcceptMessage(message);
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
    IRoutine::Cancel();
    CloseQueue();
    m_selfThread.Join();
  }

  void BackendRendererImpl::ThreadMain()
  {
    m_contextFactory->getResourcesUploadContext()->makeCurrent();

    while (!IsCancelled())
      ProcessSingleMessage(true);

    ReleaseResources();
  }

  void BackendRendererImpl::ReleaseResources()
  {
    m_threadPool->Stop();
    m_threadPool.Destroy();
    m_batchersPool.Destroy();

    GetRangeDeletor(m_taskIndex, DeleteFunctor())();
  }

  void BackendRendererImpl::Do()
  {
    ThreadMain();
  }

  void BackendRendererImpl::PostToRenderThreads(TransferPointer<Message> message)
  {
    m_commutator->PostMessage(ThreadsCommutator::RenderThread, message);
  }
}
