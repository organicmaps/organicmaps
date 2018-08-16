#include "drape_frontend/gui/drape_gui.hpp"

#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/batchers_pool.hpp"
#include "drape_frontend/circles_pack_shape.hpp"
#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/drape_measurer.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/read_manager.hpp"
#include "drape_frontend/route_builder.hpp"
#include "drape_frontend/user_mark_shapes.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/texture_manager.hpp"

#include "indexer/scales.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <utility>

using namespace std::placeholders;

namespace df
{
BackendRenderer::BackendRenderer(Params && params)
  : BaseRenderer(ThreadsCommutator::ResourceUploadThread, params)
  , m_model(params.m_model)
  , m_readManager(make_unique_dp<ReadManager>(params.m_commutator, m_model,
                                              params.m_allow3dBuildings, params.m_trafficEnabled,
                                              std::move(params.m_isUGCFn)))
  , m_transitBuilder(make_unique_dp<TransitSchemeBuilder>(
        std::bind(&BackendRenderer::FlushTransitRenderData, this, _1)))
  , m_trafficGenerator(make_unique_dp<TrafficGenerator>(
        std::bind(&BackendRenderer::FlushTrafficRenderData, this, _1)))
  , m_userMarkGenerator(make_unique_dp<UserMarkGenerator>(
        std::bind(&BackendRenderer::FlushUserMarksRenderData, this, _1)))
  , m_requestedTiles(params.m_requestedTiles)
  , m_updateCurrentCountryFn(params.m_updateCurrentCountryFn)
  , m_metalineManager(make_unique_dp<MetalineManager>(params.m_commutator, m_model))
{
#ifdef DEBUG
  m_isTeardowned = false;
#endif

  TrafficGenerator::SetSimplifiedColorSchemeEnabled(params.m_simplifiedTrafficColors);

  ASSERT(m_updateCurrentCountryFn != nullptr, ());

  m_routeBuilder = make_unique_dp<RouteBuilder>([this](drape_ptr<SubrouteData> && subrouteData)
  {
    m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                              make_unique_dp<FlushSubrouteMessage>(std::move(subrouteData)),
                              MessagePriority::Normal);
  }, [this](drape_ptr<SubrouteArrowsData> && subrouteArrowsData)
  {
    m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                              make_unique_dp<FlushSubrouteArrowsMessage>(std::move(subrouteArrowsData)),
                              MessagePriority::Normal);
  }, [this](drape_ptr<SubrouteMarkersData> && subrouteMarkersData)
  {
    m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                              make_unique_dp<FlushSubrouteMarkersMessage>(std::move(subrouteMarkersData)),
                              MessagePriority::Normal);
  });

  StartThread();
}

BackendRenderer::~BackendRenderer()
{
  ASSERT(m_isTeardowned, ());
}

void BackendRenderer::Teardown()
{
  StopThread();
#ifdef DEBUG
  m_isTeardowned = true;
#endif
}

unique_ptr<threads::IRoutine> BackendRenderer::CreateRoutine()
{
  return make_unique<Routine>(*this);
}

void BackendRenderer::RecacheGui(gui::TWidgetsInitInfo const & initInfo, bool needResetOldGui)
{
  auto context = make_ref(m_contextFactory->GetResourcesUploadContext());
  drape_ptr<gui::LayerRenderer> layerRenderer = m_guiCacher.RecacheWidgets(context, initInfo, m_texMng);
  drape_ptr<Message> outputMsg = make_unique_dp<GuiLayerRecachedMessage>(std::move(layerRenderer), needResetOldGui);
  m_commutator->PostMessage(ThreadsCommutator::RenderThread, std::move(outputMsg), MessagePriority::Normal);
}

#ifdef RENDER_DEBUG_INFO_LABELS
void BackendRenderer::RecacheDebugLabels()
{
  auto context = make_ref(m_contextFactory->GetResourcesUploadContext());
  drape_ptr<gui::LayerRenderer> layerRenderer = m_guiCacher.RecacheDebugLabels(context, m_texMng);
  drape_ptr<Message> outputMsg = make_unique_dp<GuiLayerRecachedMessage>(std::move(layerRenderer), false);
  m_commutator->PostMessage(ThreadsCommutator::RenderThread, std::move(outputMsg), MessagePriority::Normal);
}
#endif

void BackendRenderer::RecacheChoosePositionMark()
{
  auto context = make_ref(m_contextFactory->GetResourcesUploadContext());
  drape_ptr<gui::LayerRenderer> layerRenderer = m_guiCacher.RecacheChoosePositionMark(context, m_texMng);
  drape_ptr<Message> outputMsg = make_unique_dp<GuiLayerRecachedMessage>(std::move(layerRenderer), false);
  m_commutator->PostMessage(ThreadsCommutator::RenderThread, std::move(outputMsg), MessagePriority::Normal);
}

void BackendRenderer::AcceptMessage(ref_ptr<Message> message)
{
  switch (message->GetType())
  {
  case Message::UpdateReadManager:
    {
      TTilesCollection tiles = m_requestedTiles->GetTiles();
      if (!tiles.empty())
      {
        ScreenBase screen;
        bool have3dBuildings;
        bool forceRequest;
        bool forceUserMarksRequest;
        m_requestedTiles->GetParams(screen, have3dBuildings, forceRequest, forceUserMarksRequest);
        m_readManager->UpdateCoverage(screen, have3dBuildings, forceRequest, forceUserMarksRequest,
                                      tiles, m_texMng, make_ref(m_metalineManager));
        m_updateCurrentCountryFn(screen.ClipRect().Center(), (*tiles.begin()).m_zoomLevel);
      }
      break;
    }

  case Message::InvalidateReadManagerRect:
    {
      ref_ptr<InvalidateReadManagerRectMessage> msg = message;
      if (msg->NeedRestartReading())
        m_readManager->Restart();
      else
        m_readManager->Invalidate(msg->GetTilesForInvalidate());
      break;
    }

  case Message::ShowChoosePositionMark:
    {
      RecacheChoosePositionMark();
      break;
    }

  case Message::GuiRecache:
    {
      ref_ptr<GuiRecacheMessage> msg = message;
      m_lastWidgetsInfo = msg->GetInitInfo();
      RecacheGui(m_lastWidgetsInfo, msg->NeedResetOldGui());
#ifdef RENDER_DEBUG_INFO_LABELS
      RecacheDebugLabels();
#endif
      break;
    }

  case Message::GuiLayerLayout:
    {
      ref_ptr<GuiLayerLayoutMessage> msg = message;
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<GuiLayerLayoutMessage>(msg->AcceptLayoutInfo()),
                                MessagePriority::Normal);
      break;
    }

  case Message::TileReadStarted:
    {
      ref_ptr<TileReadStartMessage> msg = message;
      m_batchersPool->ReserveBatcher(msg->GetKey());
      break;
    }

  case Message::TileReadEnded:
    {
      ref_ptr<TileReadEndMessage> msg = message;
      m_batchersPool->ReleaseBatcher(msg->GetKey());
      break;
    }

  case Message::FinishTileRead:
    {
      ref_ptr<FinishTileReadMessage> msg = message;
      if (msg->NeedForceUpdateUserMarks())
      {
        for (auto const & tileKey : msg->GetTiles())
          m_userMarkGenerator->GenerateUserMarksGeometry(tileKey, m_texMng);
      }
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<FinishTileReadMessage>(msg->MoveTiles(),
                                                                      msg->NeedForceUpdateUserMarks()),
                                MessagePriority::Normal);
      break;
    }

  case Message::FinishReading:
    {
      TOverlaysRenderData overlays;
      overlays.swap(m_overlays);
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<FlushOverlaysMessage>(std::move(overlays)),
                                MessagePriority::Normal);
      break;
    }

  case Message::MapShapesRecache:
    {
      RecacheMapShapes();
      break;
    }

  case Message::MapShapeReaded:
    {
      ref_ptr<MapShapeReadedMessage> msg = message;
      auto const & tileKey = msg->GetKey();
      if (m_requestedTiles->CheckTileKey(tileKey) && m_readManager->CheckTileKey(tileKey))
      {
        ref_ptr<dp::Batcher> batcher = m_batchersPool->GetBatcher(tileKey);
#if defined(DRAPE_MEASURER) && defined(GENERATING_STATISTIC)
        DrapeMeasurer::Instance().StartShapesGeneration();
#endif
        for (drape_ptr<MapShape> const & shape : msg->GetShapes())
        {
          batcher->SetFeatureMinZoom(shape->GetFeatureMinZoom());
          shape->Draw(batcher, m_texMng);
        }
#if defined(DRAPE_MEASURER) && defined(GENERATING_STATISTIC)
        DrapeMeasurer::Instance().EndShapesGeneration(static_cast<uint32_t>(msg->GetShapes().size()));
#endif
      }
      break;
    }

  case Message::OverlayMapShapeReaded:
    {
      ref_ptr<OverlayMapShapeReadedMessage> msg = message;
      auto const & tileKey = msg->GetKey();
      if (m_requestedTiles->CheckTileKey(tileKey) && m_readManager->CheckTileKey(tileKey))
      {
        CleanupOverlays(tileKey);

#if defined(DRAPE_MEASURER) && defined(GENERATING_STATISTIC)
        DrapeMeasurer::Instance().StartOverlayShapesGeneration();
#endif
        OverlayBatcher batcher(tileKey);
        for (drape_ptr<MapShape> const & shape : msg->GetShapes())
          batcher.Batch(shape, m_texMng);

        TOverlaysRenderData renderData;
        batcher.Finish(renderData);
        if (!renderData.empty())
        {
          m_overlays.reserve(m_overlays.size() + renderData.size());
          std::move(renderData.begin(), renderData.end(), back_inserter(m_overlays));
        }

#if defined(DRAPE_MEASURER) && defined(GENERATING_STATISTIC)
        DrapeMeasurer::Instance().EndOverlayShapesGeneration(
              static_cast<uint32_t>(msg->GetShapes().size()));
#endif
      }
      break;
    }

  case Message::ChangeUserMarkGroupVisibility:
    {
      ref_ptr<ChangeUserMarkGroupVisibilityMessage> msg = message;
      m_userMarkGenerator->SetGroupVisibility(msg->GetGroupId(), msg->IsVisible());
      break;
    }

  case Message::UpdateUserMarks:
    {
      ref_ptr<UpdateUserMarksMessage> msg = message;
      m_userMarkGenerator->SetRemovedUserMarks(msg->AcceptRemovedIds());
      m_userMarkGenerator->SetUserMarks(msg->AcceptMarkRenderParams());
      m_userMarkGenerator->SetUserLines(msg->AcceptLineRenderParams());
      m_userMarkGenerator->SetCreatedUserMarks(msg->AcceptCreatedIds());
      break;
    }

  case Message::UpdateUserMarkGroup:
    {
      ref_ptr<UpdateUserMarkGroupMessage> msg = message;
      kml::MarkGroupId const groupId = msg->GetGroupId();
      m_userMarkGenerator->SetGroup(groupId, msg->AcceptIds());
      break;
    }

  case Message::ClearUserMarkGroup:
    {
      ref_ptr<ClearUserMarkGroupMessage> msg = message;
      m_userMarkGenerator->RemoveGroup(msg->GetGroupId());
      break;
    }

  case Message::InvalidateUserMarks:
    {
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<InvalidateUserMarksMessage>(),
                                MessagePriority::Normal);
      break;
    }
  case Message::AddSubroute:
    {
      ref_ptr<AddSubrouteMessage> msg = message;
      auto context = make_ref(m_contextFactory->GetResourcesUploadContext());
      m_routeBuilder->Build(context, msg->GetSubrouteId(), msg->GetSubroute(), m_texMng,
                            msg->GetRecacheId());
      break;
    }

  case Message::CacheSubrouteArrows:
    {
      ref_ptr<CacheSubrouteArrowsMessage> msg = message;
      auto context = make_ref(m_contextFactory->GetResourcesUploadContext());
      m_routeBuilder->BuildArrows(context, msg->GetSubrouteId(), msg->GetBorders(), m_texMng,
                                  msg->GetRecacheId());
      break;
    }

  case Message::RemoveSubroute:
    {
      ref_ptr<RemoveSubrouteMessage> msg = message;
      m_routeBuilder->ClearRouteCache();
      // We have to resend the message to FR, because it guaranties that
      // RemoveSubroute will be processed after FlushSubrouteMessage.
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<RemoveSubrouteMessage>(
                                  msg->GetSegmentId(), msg->NeedDeactivateFollowing()),
                                MessagePriority::Normal);
      break;
    }

  case Message::SwitchMapStyle:
    {
      m_texMng->OnSwitchMapStyle();
      RecacheMapShapes();
      RecacheGui(m_lastWidgetsInfo, false /* needResetOldGui */);
#ifdef RENDER_DEBUG_INFO_LABELS
      RecacheDebugLabels();
#endif
      m_trafficGenerator->InvalidateTexturesCache();
      m_transitBuilder->RebuildSchemes(m_texMng);
      break;
    }

  case Message::CacheCirclesPack:
    {
      ref_ptr<CacheCirclesPackMessage> msg = message;
      drape_ptr<CirclesPackRenderData> data = make_unique_dp<CirclesPackRenderData>();
      data->m_pointsCount = msg->GetPointsCount();
      auto context = make_ref(m_contextFactory->GetResourcesUploadContext());
      CirclesPackShape::Draw(context, m_texMng, *data.get());
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<FlushCirclesPackMessage>(
                                  std::move(data), msg->GetDestination()),
                                MessagePriority::Normal);
      break;
    }

  case Message::Allow3dBuildings:
    {
      ref_ptr<Allow3dBuildingsMessage> msg = message;
      m_readManager->Allow3dBuildings(msg->Allow3dBuildings());
      break;
    }

  case Message::RequestSymbolsSize:
    {
      ref_ptr<RequestSymbolsSizeMessage> msg = message;
      auto const & symbols = msg->GetSymbols();

      RequestSymbolsSizeMessage::Sizes sizes;
      for (auto const & s : symbols)
      {
        dp::TextureManager::SymbolRegion region;
        m_texMng->GetSymbolRegion(s, region);
        sizes.insert(std::make_pair(s, region.GetPixelSize()));
      }
      msg->InvokeCallback(std::move(sizes));

      break;
    }

  case Message::EnableTraffic:
    {
      ref_ptr<EnableTrafficMessage> msg = message;
      if (!msg->IsTrafficEnabled())
        m_trafficGenerator->ClearCache();
      m_readManager->SetTrafficEnabled(msg->IsTrafficEnabled());
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<EnableTrafficMessage>(msg->IsTrafficEnabled()),
                                MessagePriority::Normal);
      break;
    }

  case Message::FlushTrafficGeometry:
    {
      ref_ptr<FlushTrafficGeometryMessage> msg = message;
      auto const & tileKey = msg->GetKey();
      if (m_requestedTiles->CheckTileKey(tileKey) && m_readManager->CheckTileKey(tileKey))
      {
        auto context = make_ref(m_contextFactory->GetResourcesUploadContext());
        m_trafficGenerator->FlushSegmentsGeometry(context, tileKey, msg->GetSegments(), m_texMng);
      }
      break;
    }

  case Message::UpdateTraffic:
    {
      ref_ptr<UpdateTrafficMessage> msg = message;
      m_trafficGenerator->UpdateColoring(msg->GetSegmentsColoring());
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<RegenerateTrafficMessage>(),
                                MessagePriority::Normal);
      break;
    }

  case Message::ClearTrafficData:
    {
      ref_ptr<ClearTrafficDataMessage> msg = message;

      m_trafficGenerator->ClearCache(msg->GetMwmId());

      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<ClearTrafficDataMessage>(msg->GetMwmId()),
                                MessagePriority::Normal);
      break;
    }

  case Message::SetSimplifiedTrafficColors:
    {
      ref_ptr<SetSimplifiedTrafficColorsMessage> msg = message;

      m_trafficGenerator->SetSimplifiedColorSchemeEnabled(msg->IsSimplified());
      m_trafficGenerator->InvalidateTexturesCache();

      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<SetSimplifiedTrafficColorsMessage>(msg->IsSimplified()),
                                MessagePriority::Normal);
      break;
    }

  case Message::UpdateTransitScheme:
    {
      ref_ptr<UpdateTransitSchemeMessage> msg = message;
      m_transitBuilder->UpdateSchemes(msg->GetTransitDisplayInfos(), m_texMng);
      break;
    }

  case Message::RegenerateTransitScheme:
    {
      m_transitBuilder->RebuildSchemes(m_texMng);
      break;
    }

  case Message::ClearTransitSchemeData:
    {
      ref_ptr<ClearTransitSchemeDataMessage> msg = message;
      m_transitBuilder->Clear(msg->GetMwmId());
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<ClearTransitSchemeDataMessage>(msg->GetMwmId()),
                                MessagePriority::Normal);
      break;
    }

  case Message::ClearAllTransitSchemeData:
    {
      m_transitBuilder->Clear();
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<ClearAllTransitSchemeDataMessage>(),
                                MessagePriority::Normal);
      break;
    }

  case Message::EnableTransitScheme:
    {
      ref_ptr<EnableTransitSchemeMessage> msg = message;
      if (!msg->IsEnabled())
        m_transitBuilder->Clear();
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<EnableTransitSchemeMessage>(msg->IsEnabled()),
                                MessagePriority::Normal);
      break;
    }
  case Message::DrapeApiAddLines:
    {
      ref_ptr<DrapeApiAddLinesMessage> msg = message;
      vector<drape_ptr<DrapeApiRenderProperty>> properties;
      m_drapeApiBuilder->BuildLines(msg->GetLines(), m_texMng, properties);
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<DrapeApiFlushMessage>(std::move(properties)),
                                MessagePriority::Normal);
      break;
    }

  case Message::DrapeApiRemove:
    {
      ref_ptr<DrapeApiRemoveMessage> msg = message;
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<DrapeApiRemoveMessage>(msg->GetId(), msg->NeedRemoveAll()),
                                MessagePriority::Normal);
      break;
    }

  case Message::SetCustomFeatures:
    {
      ref_ptr<SetCustomFeaturesMessage> msg = message;
      m_readManager->SetCustomFeatures(msg->AcceptFeatures());
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<SetTrackedFeaturesMessage>(
                                  m_readManager->GetCustomFeaturesArray()),
                                MessagePriority::Normal);
      break;
    }

  case Message::RemoveCustomFeatures:
    {
      ref_ptr<RemoveCustomFeaturesMessage> msg = message;
      bool changed = false;
      if (msg->NeedRemoveAll())
        changed = m_readManager->RemoveAllCustomFeatures();
      else
        changed = m_readManager->RemoveCustomFeatures(msg->GetMwmId());

      if (changed)
      {
        m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SetTrackedFeaturesMessage>(
                                    m_readManager->GetCustomFeaturesArray()),
                                  MessagePriority::Normal);
      }
      break;
    }

  case Message::SetDisplacementMode:
    {
      ref_ptr<SetDisplacementModeMessage> msg = message;
      m_readManager->SetDisplacementMode(msg->GetMode());
      if (m_readManager->IsModeChanged())
      {
        m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SetDisplacementModeMessage>(msg->GetMode()),
                                  MessagePriority::Normal);
      }
      break;
    }

  case Message::EnableUGCRendering:
    {
      ref_ptr<EnableUGCRenderingMessage> msg = message;
      m_readManager->EnableUGCRendering(msg->IsEnabled());
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<EnableUGCRenderingMessage>(msg->IsEnabled()),
                                MessagePriority::Normal);
      break;
    }

  case Message::NotifyRenderThread:
    {
      ref_ptr<NotifyRenderThreadMessage> msg = message;
      msg->InvokeFunctor();
      break;
    }

  default:
    ASSERT(false, ());
    break;
  }
}

void BackendRenderer::ReleaseResources()
{
  m_readManager->Stop();

  m_readManager.reset();
  m_metalineManager.reset();
  m_batchersPool.reset();
  m_routeBuilder.reset();
  m_overlays.clear();
  m_trafficGenerator.reset();

  m_texMng->Release();
  m_contextFactory->GetResourcesUploadContext()->DoneCurrent();
}

void BackendRenderer::OnContextCreate()
{
  LOG(LINFO, ("On context create."));
  auto context = m_contextFactory->GetResourcesUploadContext();
  m_contextFactory->WaitForInitialization(context);
  context->MakeCurrent();
  context->Init(m_apiVersion);

  m_readManager->Start();
  InitGLDependentResource();
}

void BackendRenderer::OnContextDestroy()
{
  LOG(LINFO, ("On context destroy."));
  m_readManager->Stop();
  m_batchersPool.reset();
  m_metalineManager->Stop();
  m_texMng->Release();
  m_overlays.clear();
  m_trafficGenerator->ClearGLDependentResources();

  m_contextFactory->GetResourcesUploadContext()->DoneCurrent();
}

BackendRenderer::Routine::Routine(BackendRenderer & renderer) : m_renderer(renderer) {}

void BackendRenderer::Routine::Do()
{
  LOG(LINFO, ("Start routine."));
  m_renderer.OnContextCreate();
  dp::GraphicsContext * context = m_renderer.m_contextFactory->GetResourcesUploadContext();
  while (!IsCancelled())
  {
    if (context->Validate())
      m_renderer.ProcessSingleMessage();
    m_renderer.CheckRenderingEnabled();
  }

  m_renderer.ReleaseResources();
}

void BackendRenderer::InitGLDependentResource()
{
  uint32_t constexpr kBatchSize = 5000;
  m_batchersPool = make_unique_dp<BatchersPool<TileKey, TileKeyStrictComparator>>(kReadingThreadsCount,
                                               std::bind(&BackendRenderer::FlushGeometry, this, _1, _2, _3),
                                               kBatchSize, kBatchSize);
  m_trafficGenerator->Init();

  dp::TextureManager::Params params;
  params.m_resPostfix = VisualParams::Instance().GetResourcePostfix();
  params.m_visualScale = df::VisualParams::Instance().GetVisualScale();
#ifdef BUILD_DESIGNER
  params.m_colors = "colors_design.txt";
  params.m_patterns = "patterns_design.txt";
#else
  params.m_colors = "colors.txt";
  params.m_patterns = "patterns.txt";
#endif // BUILD_DESIGNER
  params.m_glyphMngParams.m_uniBlocks = "unicode_blocks.txt";
  params.m_glyphMngParams.m_whitelist = "fonts_whitelist.txt";
  params.m_glyphMngParams.m_blacklist = "fonts_blacklist.txt";
  params.m_glyphMngParams.m_sdfScale = VisualParams::Instance().GetGlyphSdfScale();
  params.m_glyphMngParams.m_baseGlyphHeight = VisualParams::Instance().GetGlyphBaseSize();
  GetPlatform().GetFontNames(params.m_glyphMngParams.m_fonts);

  m_texMng->Init(params);

  // Send some textures to frontend renderer.
  drape_ptr<PostprocessStaticTextures> textures = make_unique_dp<PostprocessStaticTextures>();
  textures->m_smaaAreaTexture = m_texMng->GetSMAAAreaTexture();
  textures->m_smaaSearchTexture = m_texMng->GetSMAASearchTexture();
  m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                            make_unique_dp<SetPostprocessStaticTexturesMessage>(std::move(textures)),
                            MessagePriority::Normal);

  m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                            make_unique_dp<FinishTexturesInitializationMessage>(),
                            MessagePriority::Normal);
}

void BackendRenderer::RecacheMapShapes()
{
  auto msg = make_unique_dp<MapShapesMessage>(make_unique_dp<MyPosition>(m_texMng),
                                              make_unique_dp<SelectionShape>(m_texMng));
  m_contextFactory->GetResourcesUploadContext()->Flush();
  m_commutator->PostMessage(ThreadsCommutator::RenderThread, std::move(msg), MessagePriority::Normal);
}

void BackendRenderer::FlushGeometry(TileKey const & key, dp::RenderState const & state,
                                    drape_ptr<dp::RenderBucket> && buffer)
{
  m_contextFactory->GetResourcesUploadContext()->Flush();
  m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                            make_unique_dp<FlushRenderBucketMessage>(key, state, std::move(buffer)),
                            MessagePriority::Normal);
}

void BackendRenderer::FlushTransitRenderData(TransitRenderData && renderData)
{
  m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                            make_unique_dp<FlushTransitSchemeMessage>(std::move(renderData)),
                            MessagePriority::Normal);
}

void BackendRenderer::FlushTrafficRenderData(TrafficRenderData && renderData)
{
  m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                            make_unique_dp<FlushTrafficDataMessage>(std::move(renderData)),
                            MessagePriority::Normal);
}

void BackendRenderer::FlushUserMarksRenderData(TUserMarksRenderData && renderData)
{
  m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                            make_unique_dp<FlushUserMarksMessage>(std::move(renderData)),
                            MessagePriority::Normal);
}

void BackendRenderer::CleanupOverlays(TileKey const & tileKey)
{
  auto const functor = [&tileKey](OverlayRenderData const & data)
  {
    return data.m_tileKey == tileKey && data.m_tileKey.m_generation < tileKey.m_generation;
  };
  m_overlays.erase(std::remove_if(m_overlays.begin(), m_overlays.end(), functor), m_overlays.end());
}
}  // namespace df
