#include "backend_renderer.hpp"

#include "threads_commutator.hpp"
#include "tile_info.hpp"
#include "message_subclasses.hpp"

#include "../geometry/screenbase.hpp"

#include "../platform/platform.hpp"

#include "../base/buffer_vector.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"

#include "../indexer/mercator.hpp"

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
  BackendRenderer::BackendRenderer(RefPointer<ThreadsCommutator> commutator,
                                   RefPointer<OGLContextFactory> oglcontextfactory,
                                   double visualScale,
                                   int surfaceWidth,
                                   int surfaceHeight)
    : m_engineContext(commutator)
    , m_commutator(commutator)
    , m_contextFactory(oglcontextfactory)
  {
    ///{ Temporary initialization
    Platform::FilesList maps;
    Platform & pl = GetPlatform();
    pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, maps);

    for_each(maps.begin(), maps.end(), bind(&model::FeaturesFetcher::AddMap, &m_model, _1));
    ///}


    m_scaleProcessor.SetParams(visualScale, ScalesProcessor::CalculateTileSize(surfaceWidth, surfaceHeight));
    m_currentViewport.SetFromRect(m2::AnyRectD(m_scaleProcessor.GetWorldRect()));

    m_commutator->RegisterThread(ThreadsCommutator::ResourceUploadThread, this);

    int readerCount = 1;//max(1, GetPlatform().CpuCores() - 2);
    m_threadPool.Reset(new threads::ThreadPool(readerCount, bind(&PostFinishTask, commutator, _1)));
    m_batchersPool.Reset(new BatchersPool(readerCount, bind(&BackendRenderer::PostToRenderThreads, this, _1)));

    StartThread();
  }

  BackendRenderer::~BackendRenderer()
  {
    StopThread();
  }

  void BackendRenderer::UpdateCoverage(ScreenBase const & screen)
  {
    set<TileKey> tilesKeysSet;
    GetTileKeys(tilesKeysSet, screen);

    if (!m_currentViewport.GlobalRect().IsIntersect(screen.GlobalRect()) ||
        m_scaleProcessor.GetTileScaleBase(m_currentViewport) != m_scaleProcessor.GetTileScaleBase(screen))
    {

      typedef set<ReadMWMTask *>::iterator index_iter;
      for (index_iter it = m_taskIndex.begin(); it != m_taskIndex.end(); ++it)
        CancelTask(*it, false, false);

      m_taskIndex.clear();
      PostToRenderThreads(MovePointer<Message>(new DropCoverageMessage()));

      typedef set<TileKey>::iterator tile_keys_iterator;
      for (tile_keys_iterator it = tilesKeysSet.begin(); it != tilesKeysSet.end(); ++it)
        CreateTask(*it);
    }
    else
    {
      // Find rects that go out from viewport
      buffer_vector<ReadMWMTask *, 8> outdatedTasks;
      set_difference(m_taskIndex.begin(), m_taskIndex.end(),
                     tilesKeysSet.begin(), tilesKeysSet.end(),
                     back_inserter(outdatedTasks), CoverageCellComparer());

      // Find rects that go in into viewport
      buffer_vector<TileKey, 8> inputRects;
      set_difference(tilesKeysSet.begin(), tilesKeysSet.end(),
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

  void BackendRenderer::FinishTask(threads::IRoutine * routine)
  {
    ReadMWMTask * readTask = static_cast<ReadMWMTask *>(routine);
    readTask->Finish();
    if (routine->IsCancelled())
    {
      ASSERT(m_taskIndex.find(readTask) == m_taskIndex.end(), ());
      CancelTask(readTask, false, true);
    }
  }

  void BackendRenderer::Resize(m2::RectI const & rect)
  {
    m_currentViewport.OnSize(rect);
    UpdateCoverage(m_currentViewport);
  }

  void BackendRenderer::CreateTask(TileKey const & info)
  {
    ReadMWMTask * task = new ReadMWMTask(info, m_model, m_index, m_engineContext);
    m_taskIndex.insert(task);
    m_threadPool->AddTask(task);
  }

  void BackendRenderer::CancelTask(ReadMWMTask * task, bool removefromIndex, bool postToRenderer)
  {
    task->Cancel();

    if (postToRenderer)
      PostToRenderThreads(MovePointer<Message>(new DropTileMessage(task->GetTileInfo().m_key)));

    if (removefromIndex)
      m_taskIndex.erase(task);

    if (task->IsFinished())
      delete task;
  }

  void BackendRenderer::RestartExistTasks()
  {
    typedef set<ReadMWMTask *>::iterator index_iter;
    for (index_iter it = m_taskIndex.begin(); it != m_taskIndex.end(); ++it)
    {
      (*it)->PrepareToRestart();
      m_threadPool->AddTask(*it);
    }
  }

  void BackendRenderer::GetTileKeys(set<TileKey> & out, ScreenBase const & screen)
  {
    out.clear();

    int const tileScale = m_scaleProcessor.GetTileScaleBase(screen);
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

  /////////////////////////////////////////
  //           MessageAcceptor           //
  /////////////////////////////////////////
  void BackendRenderer::AcceptMessage(RefPointer<Message> message)
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
  void BackendRenderer::StartThread()
  {
    m_selfThread.Create(this);
  }

  void BackendRenderer::StopThread()
  {
    IRoutine::Cancel();
    CloseQueue();
    m_selfThread.Join();
  }

  void BackendRenderer::ThreadMain()
  {
    m_contextFactory->getResourcesUploadContext()->makeCurrent();

    while (!IsCancelled())
      ProcessSingleMessage(true);

    ReleaseResources();
  }

  void BackendRenderer::ReleaseResources()
  {
    m_threadPool->Stop();
    m_threadPool.Destroy();
    m_batchersPool.Destroy();

    GetRangeDeletor(m_taskIndex, DeleteFunctor())();
  }

  void BackendRenderer::Do()
  {
    ThreadMain();
  }

  void BackendRenderer::PostToRenderThreads(TransferPointer<Message> message)
  {
    m_commutator->PostMessage(ThreadsCommutator::RenderThread, message);
  }
}
