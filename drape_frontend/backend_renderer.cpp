#include "backend_renderer.hpp"
#include "read_manager.hpp"
#include "batchers_pool.hpp"
#include "visual_params.hpp"
#include "map_shape.hpp"

#include "threads_commutator.hpp"
#include "message_subclasses.hpp"

#include "../drape/texture_set_holder.hpp"
#include "../drape/oglcontextfactory.hpp"

#include "../platform/platform.hpp"

#include "../std/bind.hpp"


namespace df
{
  BackendRenderer::BackendRenderer(RefPointer<ThreadsCommutator> commutator,
                                   RefPointer<OGLContextFactory> oglcontextfactory,
                                   RefPointer<TextureSetHolder> textureHolder)
    : m_engineContext(commutator)
    , m_commutator(commutator)
    , m_contextFactory(oglcontextfactory)
    , m_textures(textureHolder)
  {
    ///{ Temporary initialization
    m_model.InitClassificator();
    Platform::FilesList maps;
    Platform & pl = GetPlatform();
    pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, maps);

    for_each(maps.begin(), maps.end(), bind(&model::FeaturesFetcher::AddMap, &m_model, _1));
    ///}

    m_commutator->RegisterThread(ThreadsCommutator::ResourceUploadThread, this);
    m_batchersPool.Reset(new BatchersPool(ReadManager::ReadCount(), bind(&BackendRenderer::PostToRenderThreads, this, _1)));
    m_readManager.Reset(new ReadManager(m_engineContext, m_model));

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
        m_readManager->UpdateCoverage(screen);
      }
      break;
    case Message::TileReadStarted:
      m_batchersPool->ReserveBatcher(static_cast<BaseTileMessage *>(message.GetRaw())->GetKey());
      break;
    case Message::TileReadEnded:
      m_batchersPool->ReleaseBatcher(static_cast<BaseTileMessage *>(message.GetRaw())->GetKey());
      break;
    case Message::MapShapeReaded:
      {
        MapShapeReadedMessage * msg = static_cast<MapShapeReadedMessage *>(message.GetRaw());
        RefPointer<Batcher> batcher = m_batchersPool->GetTileBatcher(msg->GetKey());
        MasterPointer<MapShape> shape(msg->GetShape());
        shape->Draw(batcher, m_textures);

        shape.Destroy();
      }
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

    InitGLDependentResource();

    while (!IsCancelled())
      ProcessSingleMessage(true);

    ReleaseResources();
  }

  void BackendRenderer::ReleaseResources()
  {
    m_readManager->Stop();

    m_readManager.Destroy();
    m_batchersPool.Destroy();

    m_textures->Release();
    m_textures = RefPointer<TextureSetHolder>();
  }

  void BackendRenderer::Do()
  {
    ThreadMain();
  }

  void BackendRenderer::InitGLDependentResource()
  {
    m_textures->Init(VisualParams::Instance().GetResourcePostfix());
  }

  void BackendRenderer::PostToRenderThreads(TransferPointer<Message> message)
  {
    m_commutator->PostMessage(ThreadsCommutator::RenderThread, message);
  }
}
