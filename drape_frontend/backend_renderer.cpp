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
  case Message::UpdateReadManager:
    {
      UpdateReadManagerMessage * msg = df::CastMessage<UpdateReadManagerMessage>(message);
      ScreenBase const & screen = msg->GetScreen();
      set<TileKey> const & tiles = msg->GetTiles();
      m_readManager->UpdateCoverage(screen, tiles);
      break;
    }
  case Message::InvalidateReadManagerRect:
    {
      InvalidateReadManagerRectMessage * msg = df::CastMessage<InvalidateReadManagerRectMessage>(message);
      m_readManager->Invalidate(msg->GetTilesForInvalidate());
      break;
    }
  case Message::TileReadStarted:
    m_batchersPool->ReserveBatcher(df::CastMessage<BaseTileMessage>(message)->GetKey());
    break;
  case Message::TileReadEnded:
    m_batchersPool->ReleaseBatcher(df::CastMessage<BaseTileMessage>(message)->GetKey());
    break;
  case Message::MapShapeReaded:
    {
      MapShapeReadedMessage * msg = df::CastMessage<MapShapeReadedMessage>(message);
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
    ProcessSingleMessage();

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

} // namespace df
