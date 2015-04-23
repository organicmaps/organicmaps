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

BackendRenderer::BackendRenderer(ref_ptr<ThreadsCommutator> commutator,
                                 ref_ptr<dp::OGLContextFactory> oglcontextfactory,
                                 ref_ptr<dp::TextureManager> textureManager,
                                 MapDataProvider const & model)
  : BaseRenderer(ThreadsCommutator::ResourceUploadThread, commutator, oglcontextfactory)
  , m_model(model)
  , m_batchersPool(make_unique_dp<BatchersPool>(ReadManager::ReadCount(), bind(&BackendRenderer::FlushGeometry, this, _1)))
  , m_readManager(make_unique_dp<ReadManager>(commutator, m_model))
  , m_texturesManager(textureManager)
  , m_guiCacher("default")
{
  gui::DrapeGui::Instance().SetRecacheSlot([this](gui::Skin::ElementName elements)
  {
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<GuiRecacheMessage>(elements),
                              MessagePriority::High);
  });

  StartThread();
}

BackendRenderer::~BackendRenderer()
{
  gui::DrapeGui::Instance().ClearRecacheSlot();
  StopThread();
}

unique_ptr<threads::IRoutine> BackendRenderer::CreateRoutine()
{
  return make_unique<Routine>(*this);
}

void BackendRenderer::RecacheGui(gui::Skin::ElementName elements)
{
  drape_ptr<gui::LayerRenderer> layerRenderer = m_guiCacher.Recache(elements, m_texturesManager);
  drape_ptr<Message> outputMsg = make_unique_dp<GuiLayerRecachedMessage>(move(layerRenderer));
  m_commutator->PostMessage(ThreadsCommutator::RenderThread, move(outputMsg), MessagePriority::High);
}

/////////////////////////////////////////
//           MessageAcceptor           //
/////////////////////////////////////////
void BackendRenderer::AcceptMessage(ref_ptr<Message> message)
{
  switch (message->GetType())
  {
  case Message::UpdateReadManager:
    {
      ref_ptr<UpdateReadManagerMessage> msg = static_cast<ref_ptr<UpdateReadManagerMessage>>(message);
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
      ref_ptr<ResizeMessage> msg = static_cast<ref_ptr<ResizeMessage>>(message);
      df::Viewport const & v = msg->GetViewport();
      m_guiCacher.Resize(v.GetWidth(), v.GetHeight());
      RecacheGui(gui::Skin::AllElements);
      break;
    }
  case Message::InvalidateReadManagerRect:
    {
      ref_ptr<InvalidateReadManagerRectMessage> msg = static_cast<ref_ptr<InvalidateReadManagerRectMessage>>(message);
      m_readManager->Invalidate(msg->GetTilesForInvalidate());
      break;
    }
  case Message::GuiRecache:
    RecacheGui(static_cast<ref_ptr<GuiRecacheMessage>>(message)->GetElements());
    break;
  case Message::TileReadStarted:
    {
      m_batchersPool->ReserveBatcher(static_cast<ref_ptr<BaseTileMessage>>(message)->GetKey());
      break;
    }
  case Message::TileReadEnded:
    {
      ref_ptr<TileReadEndMessage> msg = static_cast<ref_ptr<TileReadEndMessage>>(message);
      m_batchersPool->ReleaseBatcher(msg->GetKey());
      break;
    }
  case Message::FinishReading:
    {
      ref_ptr<FinishReadingMessage> msg = static_cast<ref_ptr<FinishReadingMessage>>(message);
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<FinishReadingMessage>(move(msg->MoveTiles())),
                                MessagePriority::Normal);
      break;
    }
  case Message::MapShapeReaded:
    {
      ref_ptr<MapShapeReadedMessage> msg = static_cast<ref_ptr<MapShapeReadedMessage>>(message);
      ref_ptr<dp::Batcher> batcher = m_batchersPool->GetTileBatcher(msg->GetKey());
      for (drape_ptr<MapShape> const & shape : msg->GetShapes())
        shape->Draw(batcher, m_texturesManager);
      break;
    }
  case Message::UpdateUserMarkLayer:
    {
      ref_ptr<UpdateUserMarkLayerMessage> msg = static_cast<ref_ptr<UpdateUserMarkLayerMessage>>(message);
      TileKey const & key = msg->GetKey();

      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<ClearUserMarkLayerMessage>(key),
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
void BackendRenderer::ReleaseResources()
{
  m_readManager->Stop();

  m_readManager.reset();
  m_batchersPool.reset();

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

  drape_ptr<MyPosition> position = make_unique_dp<MyPosition>(m_texturesManager);
  m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                            make_unique_dp<MyPositionShapeMessage>(move(position)),
                            MessagePriority::High);
}

void BackendRenderer::FlushGeometry(drape_ptr<Message> && message)
{
  GLFunctions::glFlush();
  m_commutator->PostMessage(ThreadsCommutator::RenderThread, move(message), MessagePriority::Normal);
}

} // namespace df
