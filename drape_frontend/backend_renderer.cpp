#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/batchers_pool.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/read_manager.hpp"
#include "drape_frontend/user_mark_shapes.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape_gui/drape_gui.hpp"

#include "indexer/scales.hpp"

#include "drape/texture_manager.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/bind.hpp"

namespace df
{

BackendRenderer::BackendRenderer(dp::RefPointer<ThreadsCommutator> commutator,
                                 dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
                                 dp::RefPointer<dp::TextureManager> textureManager,
                                 MapDataProvider const & model)
  : BaseRenderer(ThreadsCommutator::ResourceUploadThread, commutator, oglcontextfactory)
  , m_model(model)
  , m_texturesManager(textureManager)
  , m_guiCacher("default")
{
  m_batchersPool.Reset(new BatchersPool(ReadManager::ReadCount(), bind(&BackendRenderer::FlushGeometry, this, _1)));
  m_readManager.Reset(new ReadManager(commutator, m_model));

  gui::DrapeGui::Instance().SetRecacheSlot([this](gui::Skin::ElementName elements)
  {
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              dp::MovePointer<Message>(new GuiRecacheMessage(elements)),
                              MessagePriority::High);
  });

  StartThread();
}

BackendRenderer::~BackendRenderer()
{
  gui::DrapeGui::Instance().ClearRecacheSlot();
  StopThread();
}

void BackendRenderer::RecacheGui(gui::Skin::ElementName elements)
{
  using TLayerRenderer = dp::TransferPointer<gui::LayerRenderer>;
  TLayerRenderer layerRenderer = m_guiCacher.Recache(elements, m_texturesManager);
  dp::TransferPointer<Message> outputMsg = dp::MovePointer<Message>(new GuiLayerRecachedMessage(layerRenderer));
  m_commutator->PostMessage(ThreadsCommutator::RenderThread, outputMsg, MessagePriority::High);
}

/////////////////////////////////////////
//           MessageAcceptor           //
/////////////////////////////////////////
void BackendRenderer::AcceptMessage(dp::RefPointer<Message> message)
{
  switch (message->GetType())
  {
  case Message::UpdateReadManager:
    {
      UpdateReadManagerMessage * msg = df::CastMessage<UpdateReadManagerMessage>(message);
      ScreenBase const & screen = msg->GetScreen();
      TTilesCollection const & tiles = msg->GetTiles();
      m_readManager->UpdateCoverage(screen, tiles);
      storage::TIndex cnt;
      if (!tiles.empty() && (*tiles.begin()).m_zoomLevel > scales::GetUpperWorldScale())
        cnt = m_model.FindCountry(screen.ClipRect().Center());

      gui::DrapeGui::Instance().SetCountryIndex(cnt);
      break;
    }
  case Message::Resize:
    {
      ResizeMessage * msg = df::CastMessage<ResizeMessage>(message);
      df::Viewport const & v = msg->GetViewport();
      m_guiCacher.Resize(v.GetWidth(), v.GetHeight());
      RecacheGui(gui::Skin::AllElements);
      break;
    }
  case Message::InvalidateReadManagerRect:
    {
      InvalidateReadManagerRectMessage * msg = df::CastMessage<InvalidateReadManagerRectMessage>(message);
      m_readManager->Invalidate(msg->GetTilesForInvalidate());
      break;
    }
  case Message::GuiRecache:
    RecacheGui(CastMessage<GuiRecacheMessage>(message)->GetElements());
    break;
  case Message::TileReadStarted:
    {
      m_batchersPool->ReserveBatcher(df::CastMessage<BaseTileMessage>(message)->GetKey());
      break;
    }
  case Message::TileReadEnded:
    {
      TileReadEndMessage * msg = df::CastMessage<TileReadEndMessage>(message);
      m_batchersPool->ReleaseBatcher(msg->GetKey());
      break;
    }
  case Message::FinishReading:
    {
      FinishReadingMessage * msg = df::CastMessage<FinishReadingMessage>(message);
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                dp::MovePointer<df::Message>(new FinishReadingMessage(move(msg->MoveTiles()))),
                                MessagePriority::Normal);
      break;
    }
  case Message::MapShapeReaded:
    {
      MapShapeReadedMessage * msg = df::CastMessage<MapShapeReadedMessage>(message);
      dp::RefPointer<dp::Batcher> batcher = m_batchersPool->GetTileBatcher(msg->GetKey());
      dp::MasterPointer<MapShape> shape(msg->GetShape());
      shape->Draw(batcher, m_texturesManager);

      shape.Destroy();
      break;
    }
  case Message::UpdateUserMarkLayer:
    {
      UpdateUserMarkLayerMessage * msg = df::CastMessage<UpdateUserMarkLayerMessage>(message);
      TileKey const & key = msg->GetKey();

      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                dp::MovePointer<Message>(new ClearUserMarkLayerMessage(key)),
                                MessagePriority::Normal);

      m_batchersPool->ReserveBatcher(key);

      UserMarksProvider const * marksProvider = msg->StartProcess();
      if (marksProvider->IsDirty())
        CacheUserMarks(marksProvider, m_batchersPool->GetTileBatcher(key), m_texturesManager);
      msg->EndProcess();
      m_batchersPool->ReleaseBatcher(key);
      break;
    }
  case Message::StopRendering:
    {
      ProcessStopRenderingMessage();
      break;
    }
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
  m_selfThread.Create(make_unique<Routine>(*this));
}

void BackendRenderer::StopThread()
{
  m_selfThread.GetRoutine()->Cancel();
  CloseQueue();
  m_selfThread.Join();
}

void BackendRenderer::ReleaseResources()
{
  m_readManager->Stop();

  m_readManager.Destroy();
  m_batchersPool.Destroy();

  m_texturesManager->Release();
}

BackendRenderer::Routine::Routine(BackendRenderer & renderer) : m_renderer(renderer) {}

void BackendRenderer::Routine::Do()
{
  m_renderer.m_contextFactory->getResourcesUploadContext()->makeCurrent();
  GLFunctions::Init();

  m_renderer.InitGLDependentResource();

  while (!IsCancelled())
  {
    m_renderer.ProcessSingleMessage();
    m_renderer.CheckRenderingEnabled();
  }

  m_renderer.ReleaseResources();
}

void BackendRenderer::InitGLDependentResource()
{
  dp::TextureManager::Params params;
  params.m_resPrefix = VisualParams::Instance().GetResourcePostfix();
  params.m_glyphMngParams.m_uniBlocks = "unicode_blocks.txt";
  params.m_glyphMngParams.m_whitelist = "fonts_whitelist.txt";
  params.m_glyphMngParams.m_blacklist = "fonts_blacklist.txt";
  GetPlatform().GetFontNames(params.m_glyphMngParams.m_fonts);

  m_texturesManager->Init(params);

  dp::MasterPointer<MyPosition> position(new MyPosition(m_texturesManager));
  m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                            dp::MovePointer<Message>(new MyPositionShapeMessage(position.Move())),
                            MessagePriority::High);
}

void BackendRenderer::FlushGeometry(dp::TransferPointer<Message> message)
{
  GLFunctions::glFlush();
  m_commutator->PostMessage(ThreadsCommutator::RenderThread, message, MessagePriority::Normal);
}

} // namespace df
