#include "backend_renderer.hpp"
#include "read_manager.hpp"
#include "batchers_pool.hpp"

#include "threads_commutator.hpp"
#include "message_subclasses.hpp"

#include "../platform/platform.hpp"

#include "../std/bind.hpp"


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

    m_commutator->RegisterThread(ThreadsCommutator::ResourceUploadThread, this);
    m_batchersPool.Reset(new BatchersPool(ReadManager::ReadCount(), bind(&BackendRenderer::PostToRenderThreads, this, _1)));
    m_readManager.Reset(new ReadManager(visualScale, surfaceWidth, surfaceHeight, m_engineContext, m_model));

    StartThread();
  }

  BackendRenderer::~BackendRenderer()
  {
    StopThread();
  }

  /////////////////////////////////////////
  //           MessageAcceptor           //
  /////////////////////////////////////////
  void BackendRenderer::AcceptMessage(RefPointer<Message> message)
  {
    switch (message->GetType())
    {
    case Message::UpdateCoverage:
      {
        ScreenBase const & screen = static_cast<UpdateCoverageMessage *>(message.GetRaw())->GetScreen();
        CoverageUpdateDescriptor descr;
        m_readManager->UpdateCoverage(screen, descr);

        if (!descr.IsEmpty())
          PostToRenderThreads(MovePointer<Message>(new DropTilesMessage(descr)));
      }
      break;
    case Message::Resize:
      m_readManager->Resize(static_cast<ResizeMessage *>(message.GetRaw())->GetRect());
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
    m_readManager->Stop();

    m_readManager.Destroy();
    m_batchersPool.Destroy();
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
