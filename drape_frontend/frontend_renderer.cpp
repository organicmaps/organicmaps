#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/batch_merge_helper.hpp"
#include "drape_frontend/drape_measurer.hpp"
#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/gui/ruler_helper.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/scenario_manager.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/screen_quad_renderer.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/user_mark_shapes.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/debug_rect_renderer.hpp"
#include "drape/framebuffer.hpp"
#include "drape/support_manager.hpp"
#include "drape/utils/glyph_usage_tracker.hpp"
#include "drape/utils/gpu_mem_tracker.hpp"
#include "drape/utils/projection.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/scales.hpp"
#include "indexer/drawing_rules.hpp"

#include "geometry/any_rect2d.hpp"

#include "base/timer.hpp"
#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/cmath.hpp"
#include "std/chrono.hpp"

namespace df
{
namespace
{
float constexpr kIsometryAngle = static_cast<float>(math::pi) * 76.0f / 180.0f;
double const kVSyncInterval = 0.06;
//double const kVSyncInterval = 0.014;

struct MergedGroupKey
{
  dp::GLState m_state;
  TileKey m_key;

  MergedGroupKey(dp::GLState const & state, TileKey const & tileKey)
    : m_state(state), m_key(tileKey)
  {}

  bool operator <(MergedGroupKey const & other) const
  {
    if (!(m_state == other.m_state))
      return m_state < other.m_state;
    return m_key.LessStrict(other.m_key);
  }
};

template <typename ToDo>
bool RemoveGroups(ToDo & filter, vector<drape_ptr<RenderGroup>> & groups,
                  ref_ptr<dp::OverlayTree> tree)
{
  size_t startCount = groups.size();
  size_t count = startCount;
  size_t current = 0;
  while (current < count)
  {
    drape_ptr<RenderGroup> & group = groups[current];
    if (filter(group))
    {
      group->RemoveOverlay(tree);
      swap(group, groups.back());
      groups.pop_back();
      --count;
    }
    else
    {
      ++current;
    }
  }

  return startCount != count;
}

struct RemoveTilePredicate
{
  mutable bool m_deletionMark = false;
  function<bool(drape_ptr<RenderGroup> const &)> const & m_predicate;

  RemoveTilePredicate(function<bool(drape_ptr<RenderGroup> const &)> const & predicate)
    : m_predicate(predicate)
  {}

  bool operator()(drape_ptr<RenderGroup> const & group) const
  {
    if (m_predicate(group))
    {
      group->DeleteLater();
      m_deletionMark = true;
      return group->CanBeDeleted();
    }

    return false;
  }
};

} // namespace

FrontendRenderer::FrontendRenderer(Params && params)
  : BaseRenderer(ThreadsCommutator::RenderThread, params)
  , m_gpuProgramManager(new dp::GpuProgramManager())
  , m_routeRenderer(new RouteRenderer())
  , m_trafficRenderer(new TrafficRenderer())
  , m_framebuffer(new dp::Framebuffer())
  , m_screenQuadRenderer(new ScreenQuadRenderer())
  , m_gpsTrackRenderer(
        new GpsTrackRenderer(bind(&FrontendRenderer::PrepareGpsTrackPoints, this, _1)))
  , m_drapeApiRenderer(new DrapeApiRenderer())
  , m_overlayTree(new dp::OverlayTree())
  , m_enablePerspectiveInNavigation(false)
  , m_enable3dBuildings(params.m_allow3dBuildings)
  , m_isIsometry(false)
  , m_blockTapEvents(params.m_blockTapEvents)
  , m_choosePositionMode(false)
  , m_viewport(params.m_viewport)
  , m_modelViewChangedFn(params.m_modelViewChangedFn)
  , m_tapEventInfoFn(params.m_tapEventFn)
  , m_userPositionChangedFn(params.m_positionChangedFn)
  , m_requestedTiles(params.m_requestedTiles)
  , m_maxGeneration(0)
  , m_needRestoreSize(false)
  , m_trafficEnabled(params.m_trafficEnabled)
  , m_overlaysTracker(new OverlaysTracker())
  , m_overlaysShowStatsCallback(move(params.m_overlaysShowStatsCallback))
  , m_forceUpdateScene(false)
#ifdef SCENARIO_ENABLE
  , m_scenarioManager(new ScenarioManager(this))
#endif
{
#ifdef DEBUG
  m_isTeardowned = false;
#endif

  ASSERT(m_tapEventInfoFn, ());
  ASSERT(m_userPositionChangedFn, ());

  m_myPositionController.reset(new MyPositionController(move(params.m_myPositionParams)));
  StartThread();
}

FrontendRenderer::~FrontendRenderer()
{
  ASSERT(m_isTeardowned, ());
}

void FrontendRenderer::Teardown()
{
  m_scenarioManager.reset();
  StopThread();
#ifdef DEBUG
  m_isTeardowned = true;
#endif
}

void FrontendRenderer::UpdateCanBeDeletedStatus()
{
  m2::RectD const & screenRect = m_userEventStream.GetCurrentScreen().ClipRect();

  vector<m2::RectD> notFinishedTileRects;
  notFinishedTileRects.reserve(m_notFinishedTiles.size());
  for (auto const & tileKey : m_notFinishedTiles)
    notFinishedTileRects.push_back(tileKey.GetGlobalRect());

  for (RenderLayer & layer : m_layers)
  {
    for (auto & group : layer.m_renderGroups)
    {
      if (group->IsPendingOnDelete())
      {
        bool canBeDeleted = true;
        if (!notFinishedTileRects.empty())
        {
          m2::RectD const tileRect = group->GetTileKey().GetGlobalRect();
          if (tileRect.IsIntersect(screenRect))
          {
            for (auto const & notFinishedRect : notFinishedTileRects)
            {
              if (notFinishedRect.IsIntersect(tileRect))
              {
                canBeDeleted = false;
                break;
              }
            }
          }
        }
        layer.m_isDirty |= group->UpdateCanBeDeletedStatus(canBeDeleted, m_currentZoomLevel, make_ref(m_overlayTree));
      }
    }
  }
}

void FrontendRenderer::AcceptMessage(ref_ptr<Message> message)
{
  switch (message->GetType())
  {
  case Message::FlushTile:
    {
      ref_ptr<FlushRenderBucketMessage> msg = message;
      dp::GLState const & state = msg->GetState();
      TileKey const & key = msg->GetKey();
      drape_ptr<dp::RenderBucket> bucket = msg->AcceptBuffer();
      if (key.m_zoomLevel == m_currentZoomLevel && CheckTileGenerations(key))
      {
        PrepareBucket(state, bucket);
        AddToRenderGroup(state, move(bucket), key);
      }
      break;
    }

  case Message::FlushOverlays:
    {
      ref_ptr<FlushOverlaysMessage> msg = message;
      TOverlaysRenderData renderData = msg->AcceptRenderData();
      for (auto & overlayRenderData : renderData)
      {
        if (overlayRenderData.m_tileKey.m_zoomLevel == m_currentZoomLevel &&
            CheckTileGenerations(overlayRenderData.m_tileKey))
        {
          PrepareBucket(overlayRenderData.m_state, overlayRenderData.m_bucket);
          AddToRenderGroup(overlayRenderData.m_state, move(overlayRenderData.m_bucket), overlayRenderData.m_tileKey);
        }
      }
      UpdateCanBeDeletedStatus();
      break;
    }

  case Message::FinishTileRead:
    {
      ref_ptr<FinishTileReadMessage> msg = message;
      bool changed = false;
      for (auto const & tileKey : msg->GetTiles())
      {
        if (CheckTileGenerations(tileKey))
        {
          auto it = m_notFinishedTiles.find(tileKey);
          if (it != m_notFinishedTiles.end())
          {
            m_notFinishedTiles.erase(it);
            changed = true;
          }
        }
      }
      if (changed)
        UpdateCanBeDeletedStatus();

      if (m_notFinishedTiles.empty())
      {
#if defined(DRAPE_MEASURER) && defined(GENERATING_STATISTIC)
        DrapeMeasurer::Instance().EndScenePreparing();
#endif
        m_trafficRenderer->OnGeometryReady(m_currentZoomLevel);
      }
      break;
    }

  case Message::InvalidateRect:
    {
      ref_ptr<InvalidateRectMessage> m = message;
      InvalidateRect(m->GetRect());
      break;
    }

  case Message::FlushUserMarks:
    {
      ref_ptr<FlushUserMarksMessage> msg = message;
      size_t const layerId = msg->GetLayerId();
      for (UserMarkShape & shape : msg->GetShapes())
      {
        PrepareBucket(shape.m_state, shape.m_bucket);
        auto program = m_gpuProgramManager->GetProgram(shape.m_state.GetProgramIndex());
        auto program3d = m_gpuProgramManager->GetProgram(shape.m_state.GetProgram3dIndex());
        auto group = make_unique_dp<UserMarkRenderGroup>(layerId, shape.m_state, shape.m_tileKey,
                                                         move(shape.m_bucket));
        m_userMarkRenderGroups.push_back(move(group));
        m_userMarkRenderGroups.back()->SetRenderParams(program, program3d, make_ref(&m_generalUniforms));
      }
      break;
    }

  case Message::ClearUserMarkLayer:
    {
      ref_ptr<ClearUserMarkLayerMessage> msg = message;
      size_t const layerId = msg->GetLayerId();
      auto const functor = [&layerId](drape_ptr<UserMarkRenderGroup> const & g)
      {
        return g->GetLayerId() == layerId;
      };

      auto const iter = remove_if(m_userMarkRenderGroups.begin(),
                                  m_userMarkRenderGroups.end(),
                                  functor);
      m_userMarkRenderGroups.erase(iter, m_userMarkRenderGroups.end());
      break;
    }

  case Message::ChangeUserMarkLayerVisibility:
    {
      ref_ptr<ChangeUserMarkLayerVisibilityMessage> msg = message;
      if (msg->IsVisible())
        m_userMarkVisibility.insert(msg->GetLayerId());
      else
        m_userMarkVisibility.erase(msg->GetLayerId());
      break;
    }

  case Message::GuiLayerRecached:
    {
      ref_ptr<GuiLayerRecachedMessage> msg = message;
      drape_ptr<gui::LayerRenderer> renderer = move(msg->AcceptRenderer());
      renderer->Build(make_ref(m_gpuProgramManager));
      if (msg->NeedResetOldGui())
        m_guiRenderer.release();
      if (m_guiRenderer == nullptr)
        m_guiRenderer = move(renderer);
      else
        m_guiRenderer->Merge(make_ref(renderer));

      bool oldMode = m_choosePositionMode;
      m_choosePositionMode = m_guiRenderer->HasWidget(gui::WIDGET_CHOOSE_POSITION_MARK);
      if (oldMode != m_choosePositionMode)
      {
        ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
        CheckIsometryMinScale(screen);
        UpdateDisplacementEnabled();
        m_forceUpdateScene = true;
      }

      if (m_guiRenderer->HasWidget(gui::WIDGET_RULER))
        gui::DrapeGui::GetRulerHelper().Invalidate();
      break;
    }

  case Message::GuiLayerLayout:
    {
      ASSERT(m_guiRenderer != nullptr, ());
      m_guiRenderer->SetLayout(ref_ptr<GuiLayerLayoutMessage>(message)->GetLayoutInfo());
      break;
    }

  case Message::MapShapes:
    {
      ref_ptr<MapShapesMessage> msg = message;
      m_myPositionController->SetRenderShape(msg->AcceptShape());
      m_selectionShape = msg->AcceptSelection();
      if (m_selectObjectMessage != nullptr)
      {
        ProcessSelection(make_ref(m_selectObjectMessage));
        m_selectObjectMessage.reset();
        AddUserEvent(make_unique_dp<SetVisibleViewportEvent>(m_userEventStream.GetVisibleViewport()));
      }
    }
    break;

  case Message::ChangeMyPostitionMode:
    {
      ref_ptr<ChangeMyPositionModeMessage> msg = message;
      switch (msg->GetChangeType())
      {
      case ChangeMyPositionModeMessage::SwitchNextMode:
        m_myPositionController->NextMode(m_userEventStream.GetCurrentScreen());
        break;
      case ChangeMyPositionModeMessage::StopFollowing:
        m_myPositionController->StopLocationFollow();
        break;
      case ChangeMyPositionModeMessage::LoseLocation:
        m_myPositionController->LoseLocation();
        break;
      default:
        ASSERT(false, ("Unknown change type:", static_cast<int>(msg->GetChangeType())));
        break;
      }
      break;
    }

  case Message::CompassInfo:
    {
      ref_ptr<CompassInfoMessage> msg = message;
      m_myPositionController->OnCompassUpdate(msg->GetInfo(), m_userEventStream.GetCurrentScreen());
      break;
    }

  case Message::GpsInfo:
    {
#ifdef SCENARIO_ENABLE
      if (m_scenarioManager->IsRunning())
        break;
#endif
      ref_ptr<GpsInfoMessage> msg = message;
      m_myPositionController->OnLocationUpdate(msg->GetInfo(), msg->IsNavigable(),
                                               m_userEventStream.GetCurrentScreen());

      location::RouteMatchingInfo const & info = msg->GetRouteInfo();
      if (info.HasDistanceFromBegin())
      {
        m_routeRenderer->UpdateDistanceFromBegin(info.GetDistanceFromBegin());
        // Here we have to recache route arrows.
        m_routeRenderer->UpdateRoute(m_userEventStream.GetCurrentScreen(),
                                     bind(&FrontendRenderer::OnCacheRouteArrows, this, _1, _2));
      }

      break;
    }

  case Message::FindVisiblePOI:
    {
      ref_ptr<FindVisiblePOIMessage> msg = message;
      ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
      msg->SetFeatureID(GetVisiblePOI(screen.isPerspective() ? screen.PtoP3d(screen.GtoP(msg->GetPoint()))
                                                             : screen.GtoP(msg->GetPoint())));
      break;
    }

  case Message::SelectObject:
    {
      ref_ptr<SelectObjectMessage> msg = message;
      m_overlayTree->SetSelectedFeature(msg->IsDismiss() ? FeatureID() : msg->GetFeatureID());
      if (m_selectionShape == nullptr)
      {
        m_selectObjectMessage = make_unique_dp<SelectObjectMessage>(msg->GetSelectedObject(), msg->GetPosition(),
                                                                    msg->GetFeatureID(), msg->IsAnim());
        break;
      }
      ProcessSelection(msg);
      AddUserEvent(make_unique_dp<SetVisibleViewportEvent>(m_userEventStream.GetVisibleViewport()));

      break;
    }

  case Message::GetSelectedObject:
    {
      ref_ptr<GetSelectedObjectMessage> msg = message;
      if (m_selectionShape != nullptr)
        msg->SetSelectedObject(m_selectionShape->GetSelectedObject());
      else
        msg->SetSelectedObject(SelectionShape::OBJECT_EMPTY);
      break;
    }

  case Message::GetMyPosition:
    {
      ref_ptr<GetMyPositionMessage> msg = message;
      msg->SetMyPosition(m_myPositionController->IsModeHasPosition(), m_myPositionController->Position());
      break;
    }

  case Message::FlushRoute:
    {
      ref_ptr<FlushRouteMessage> msg = message;
      drape_ptr<RouteData> routeData = msg->AcceptRouteData();

      if (routeData->m_recacheId > 0 && routeData->m_recacheId < m_lastRecacheRouteId)
        break;

      m2::PointD const finishPoint = routeData->m_sourcePolyline.Back();
      m_routeRenderer->SetRouteData(move(routeData), make_ref(m_gpuProgramManager));
      // Here we have to recache route arrows.
      m_routeRenderer->UpdateRoute(m_userEventStream.GetCurrentScreen(),
                                   bind(&FrontendRenderer::OnCacheRouteArrows, this, _1, _2));

      if (!m_routeRenderer->GetFinishPoint())
      {
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<CacheRouteSignMessage>(finishPoint, false /* isStart */,
                                                                        true /* isValid */),
                                  MessagePriority::High);
      }

      if (m_pendingFollowRoute != nullptr)
      {
        FollowRoute(m_pendingFollowRoute->m_preferredZoomLevel, m_pendingFollowRoute->m_preferredZoomLevelIn3d,
                    m_pendingFollowRoute->m_enableAutoZoom);
        m_pendingFollowRoute.reset();
      }
      break;
    }

  case Message::FlushRouteSign:
    {
      ref_ptr<FlushRouteSignMessage> msg = message;
      drape_ptr<RouteSignData> routeSignData = msg->AcceptRouteSignData();

      if (routeSignData->m_recacheId > 0 && routeSignData->m_recacheId < m_lastRecacheRouteId)
        break;

      m_routeRenderer->SetRouteSign(move(routeSignData), make_ref(m_gpuProgramManager));
      break;
    }

  case Message::FlushRouteArrows:
    {
      ref_ptr<FlushRouteArrowsMessage> msg = message;
      drape_ptr<RouteArrowsData> routeArrowsData = msg->AcceptRouteArrowsData();

      if (routeArrowsData->m_recacheId > 0 && routeArrowsData->m_recacheId < m_lastRecacheRouteId)
        break;

      m_routeRenderer->SetRouteArrows(move(routeArrowsData), make_ref(m_gpuProgramManager));
      break;
    }

  case Message::RemoveRoute:
    {
      ref_ptr<RemoveRouteMessage> msg = message;
      m_routeRenderer->Clear();
      ++m_lastRecacheRouteId;
      if (msg->NeedDeactivateFollowing())
      {
        m_myPositionController->DeactivateRouting();
        m_overlayTree->SetFollowingMode(false);
        if (m_enablePerspectiveInNavigation)
          DisablePerspective();
      }
      break;
    }

  case Message::FollowRoute:
    {
      ref_ptr<FollowRouteMessage> const msg = message;

      // After night style switching or drape engine reinitialization FrontendRenderer can
      // receive FollowRoute message before FlushRoute message, so we need to postpone its processing.
      if (m_routeRenderer->GetRouteData() == nullptr)
      {
        m_pendingFollowRoute.reset(new FollowRouteData(msg->GetPreferredZoomLevel(), msg->GetPreferredZoomLevelIn3d(),
                                                       msg->EnableAutoZoom()));
        break;
      }

      FollowRoute(msg->GetPreferredZoomLevel(), msg->GetPreferredZoomLevelIn3d(), msg->EnableAutoZoom());
      break;
    }

  case Message::DeactivateRouteFollowing:
    {
      m_myPositionController->DeactivateRouting();
      m_overlayTree->SetFollowingMode(false);
      if (m_enablePerspectiveInNavigation)
        DisablePerspective();
      break;
    }

  case Message::RecoverGLResources:
    {
      UpdateGLResources();
      break;
    }

  case Message::UpdateMapStyle:
    {
      // Clear all graphics.
      for (RenderLayer & layer : m_layers)
      {
        layer.m_renderGroups.clear();
        layer.m_isDirty = false;
      }

      // Invalidate read manager.
      {
        BaseBlockingMessage::Blocker blocker;
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<InvalidateReadManagerRectMessage>(blocker),
                                  MessagePriority::High);
        blocker.Wait();
      }

      // Invalidate textures and wait for completion.
      {
        BaseBlockingMessage::Blocker blocker;
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<InvalidateTexturesMessage>(blocker),
                                  MessagePriority::High);
        blocker.Wait();
      }

      UpdateGLResources();
      break;
    }

  case Message::AllowAutoZoom:
    {
      ref_ptr<AllowAutoZoomMessage> const msg = message;
      m_myPositionController->EnableAutoZoomInRouting(msg->AllowAutoZoom());
      break;
    }

  case Message::EnablePerspective:
    {
      AddUserEvent(make_unique_dp<SetAutoPerspectiveEvent>(true /* isAutoPerspective */));
      break;
    }

  case Message::Allow3dMode:
    {
      ref_ptr<Allow3dModeMessage> const msg = message;
      ScreenBase const & screen = m_userEventStream.GetCurrentScreen();

#ifdef OMIM_OS_DESKTOP
      if (m_enablePerspectiveInNavigation == msg->AllowPerspective() &&
          m_enablePerspectiveInNavigation != screen.isPerspective())
      {
        AddUserEvent(make_unique_dp<SetAutoPerspectiveEvent>(m_enablePerspectiveInNavigation));
      }
#endif

      if (m_enable3dBuildings != msg->Allow3dBuildings())
      {
        m_enable3dBuildings = msg->Allow3dBuildings();
        CheckIsometryMinScale(screen);
        m_forceUpdateScene = true;
      }

      if (m_enablePerspectiveInNavigation != msg->AllowPerspective())
      {
        m_enablePerspectiveInNavigation = msg->AllowPerspective();
        m_myPositionController->EnablePerspectiveInRouting(m_enablePerspectiveInNavigation);
        if (m_myPositionController->IsInRouting())
        {
          AddUserEvent(make_unique_dp<SetAutoPerspectiveEvent>(m_enablePerspectiveInNavigation));
        }
      }
      break;
    }

  case Message::FlushGpsTrackPoints:
    {
      ref_ptr<FlushGpsTrackPointsMessage> msg = message;
      m_gpsTrackRenderer->AddRenderData(make_ref(m_gpuProgramManager), msg->AcceptRenderData());
      break;
    }

  case Message::UpdateGpsTrackPoints:
    {
      ref_ptr<UpdateGpsTrackPointsMessage> msg = message;
      m_gpsTrackRenderer->UpdatePoints(msg->GetPointsToAdd(), msg->GetPointsToRemove());
      break;
    }

  case Message::ClearGpsTrackPoints:
    {
      m_gpsTrackRenderer->Clear();
      break;
    }

  case Message::BlockTapEvents:
    {
      ref_ptr<BlockTapEventsMessage> msg = message;
      m_blockTapEvents = msg->NeedBlock();
      break;
    }

  case Message::SetKineticScrollEnabled:
    {
      ref_ptr<SetKineticScrollEnabledMessage> msg = message;
      m_userEventStream.SetKineticScrollEnabled(msg->IsEnabled());
      break;
    }

  case Message::SetTimeInBackground:
    {
      ref_ptr<SetTimeInBackgroundMessage> msg = message;
      m_myPositionController->SetTimeInBackground(msg->GetTime());
      break;
    }

  case Message::SetAddNewPlaceMode:
    {
      ref_ptr<SetAddNewPlaceModeMessage> msg = message;
      m_userEventStream.SetKineticScrollEnabled(msg->IsKineticScrollEnabled());
      m_dragBoundArea = msg->AcceptBoundArea();
      if (msg->IsEnabled())
      {
        if (!m_dragBoundArea.empty())
        {
          PullToBoundArea(true /* randomPlace */, true /* applyZoom */);
        }
        else
        {
          m2::PointD const pt = msg->HasPosition()? msg->GetPosition() :
                                m_userEventStream.GetCurrentScreen().GlobalRect().Center();
          int zoom = kDoNotChangeZoom;
          if (m_currentZoomLevel < scales::GetAddNewPlaceScale())
            zoom = scales::GetAddNewPlaceScale();
          AddUserEvent(make_unique_dp<SetCenterEvent>(pt, zoom, true));
        }
      }
      break;
    }

  case Message::SetDisplacementMode:
    {
      ref_ptr<SetDisplacementModeMessage> msg = message;
      m_overlayTree->SetDisplacementMode(msg->GetMode());
      break;
    }

  case Message::SetVisibleViewport:
    {
      ref_ptr<SetVisibleViewportMessage> msg = message;
      AddUserEvent(make_unique_dp<SetVisibleViewportEvent>(msg->GetRect()));
      m_myPositionController->SetVisibleViewport(msg->GetRect());
      m_myPositionController->UpdatePosition();
      break;
    }

  case Message::Invalidate:
    {
      m_myPositionController->ResetRoutingNotFollowTimer();
      m_myPositionController->ResetBlockAutoZoomTimer();
      break;
    }

  case Message::EnableTraffic:
    {
      ref_ptr<EnableTrafficMessage> msg = message;
      m_trafficEnabled = msg->IsTrafficEnabled();
      if (msg->IsTrafficEnabled())
        m_forceUpdateScene = true;
      else
        m_trafficRenderer->ClearGLDependentResources();
      break;
    }

  case Message::RegenerateTraffic:
  case Message::SetSimplifiedTrafficColors:
    {
      m_forceUpdateScene = true;
      break;
    }

  case Message::FlushTrafficData:
    {
      if (!m_trafficEnabled)
        break;
      ref_ptr<FlushTrafficDataMessage> msg = message;
      m_trafficRenderer->AddRenderData(make_ref(m_gpuProgramManager), msg->AcceptTrafficData());
      break;
    }

  case Message::ClearTrafficData:
    {
      ref_ptr<ClearTrafficDataMessage> msg = message;
      m_trafficRenderer->Clear(msg->GetMwmId());
      break;
    }

  case Message::DrapeApiFlush:
    {
      ref_ptr<DrapeApiFlushMessage> msg = message;
      m_drapeApiRenderer->AddRenderProperties(make_ref(m_gpuProgramManager), msg->AcceptProperties());
      break;
    }

  case Message::DrapeApiRemove:
    {
      ref_ptr<DrapeApiRemoveMessage> msg = message;
      if (msg->NeedRemoveAll())
        m_drapeApiRenderer->Clear();
      else
        m_drapeApiRenderer->RemoveRenderProperty(msg->GetId());
      break;
    }

  case Message::UpdateCustomSymbols:
    {
      ref_ptr<UpdateCustomSymbolsMessage> msg = message;
      m_overlaysTracker->SetTrackedOverlaysFeatures(msg->AcceptSymbolsFeatures());
      m_forceUpdateScene = true;
      break;
    }

  default:
    ASSERT(false, ());
  }
}

unique_ptr<threads::IRoutine> FrontendRenderer::CreateRoutine()
{
  return make_unique<Routine>(*this);
}

void FrontendRenderer::UpdateGLResources()
{
  ++m_lastRecacheRouteId;

  // Invalidate route.
  if (m_routeRenderer->GetStartPoint())
  {
    m2::PointD const & position = m_routeRenderer->GetStartPoint()->m_position;
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<CacheRouteSignMessage>(position, true /* isStart */,
                                                                    true /* isValid */, m_lastRecacheRouteId),
                              MessagePriority::High);
  }

  if (m_routeRenderer->GetFinishPoint())
  {
    m2::PointD const & position = m_routeRenderer->GetFinishPoint()->m_position;
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<CacheRouteSignMessage>(position, false /* isStart */,
                                                                    true /* isValid */, m_lastRecacheRouteId),
                              MessagePriority::High);
  }

  auto const & routeData = m_routeRenderer->GetRouteData();
  if (routeData != nullptr)
  {
    auto recacheRouteMsg = make_unique_dp<AddRouteMessage>(routeData->m_sourcePolyline, routeData->m_sourceTurns,
                                                           routeData->m_color, routeData->m_traffic,
                                                           routeData->m_pattern, m_lastRecacheRouteId);
    m_routeRenderer->ClearGLDependentResources();
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, move(recacheRouteMsg),
                              MessagePriority::Normal);
  }

  m_trafficRenderer->ClearGLDependentResources();

  // Request new tiles.
  ScreenBase screen = m_userEventStream.GetCurrentScreen();
  m_lastReadedModelView = screen;
  m_requestedTiles->Set(screen, m_isIsometry || screen.isPerspective(), m_forceUpdateScene, ResolveTileKeys(screen));
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                            make_unique_dp<UpdateReadManagerMessage>(),
                            MessagePriority::UberHighSingleton);

  m_gpsTrackRenderer->Update();
}

void FrontendRenderer::FollowRoute(int preferredZoomLevel, int preferredZoomLevelIn3d, bool enableAutoZoom)
{
  m_myPositionController->ActivateRouting(!m_enablePerspectiveInNavigation ? preferredZoomLevel : preferredZoomLevelIn3d,
                                          enableAutoZoom);

  if (m_enablePerspectiveInNavigation)
    AddUserEvent(make_unique_dp<SetAutoPerspectiveEvent>(true /* isAutoPerspective */));

  m_overlayTree->SetFollowingMode(true);
}

void FrontendRenderer::InvalidateRect(m2::RectD const & gRect)
{
  ScreenBase screen = m_userEventStream.GetCurrentScreen();
  m2::RectD rect = gRect;
  if (rect.Intersect(screen.ClipRect()))
  {
    // Find tiles to invalidate.
    TTilesCollection tiles;
    int const dataZoomLevel = ClipTileZoomByMaxDataZoom(m_currentZoomLevel);
    CalcTilesCoverage(rect, dataZoomLevel, [this, &rect, &tiles](int tileX, int tileY)
    {
      TileKey const key(tileX, tileY, m_currentZoomLevel);
      if (rect.IsIntersect(key.GetGlobalRect()))
        tiles.insert(key);
    });

    // Remove tiles to invalidate from screen.
    auto eraseFunction = [&tiles](drape_ptr<RenderGroup> const & group)
    {
      return tiles.find(group->GetTileKey()) != tiles.end();
    };
    for (RenderLayer & layer : m_layers)
    {
      RemoveGroups(eraseFunction, layer.m_renderGroups, make_ref(m_overlayTree));
      layer.m_isDirty = true;
    }

    // Remove tiles to invalidate from backend renderer.
    BaseBlockingMessage::Blocker blocker;
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<InvalidateReadManagerRectMessage>(blocker, tiles),
                              MessagePriority::High);
    blocker.Wait();

    // Request new tiles.
    m_lastReadedModelView = screen;
    m_requestedTiles->Set(screen, m_isIsometry || screen.isPerspective(), m_forceUpdateScene,
                          ResolveTileKeys(screen));
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<UpdateReadManagerMessage>(),
                              MessagePriority::UberHighSingleton);
  }
}

void FrontendRenderer::OnResize(ScreenBase const & screen)
{
  m2::RectD const viewportRect = screen.PixelRectIn3d();
  double const kEps = 1e-5;
  bool const viewportChanged = !m2::IsEqualSize(m_lastReadedModelView.PixelRectIn3d(), viewportRect, kEps, kEps);

  m_myPositionController->OnUpdateScreen(screen);

  if (viewportChanged)
  {
    m_myPositionController->UpdatePosition();
    m_viewport.SetViewport(0, 0, viewportRect.SizeX(), viewportRect.SizeY());
  }

  if (viewportChanged || m_needRestoreSize)
  {
    m_contextFactory->getDrawContext()->resize(viewportRect.SizeX(), viewportRect.SizeY());
    m_framebuffer->SetSize(viewportRect.SizeX(), viewportRect.SizeY());
    m_needRestoreSize = false;
  }

  RefreshProjection(screen);
  RefreshPivotTransform(screen);
}

void FrontendRenderer::AddToRenderGroup(dp::GLState const & state,
                                        drape_ptr<dp::RenderBucket> && renderBucket,
                                        TileKey const & newTile)
{
  RenderLayer::RenderLayerID id = RenderLayer::GetLayerID(state);
  RenderLayer & layer = m_layers[id];

  for (auto const & g : layer.m_renderGroups)
  {
    if (!g->IsPendingOnDelete() && g->GetState() == state && g->GetTileKey().EqualStrict(newTile))
    {
      g->AddBucket(move(renderBucket));
      layer.m_isDirty = true;
      return;
    }
  }

  drape_ptr<RenderGroup> group = make_unique_dp<RenderGroup>(state, newTile);
  ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
  ref_ptr<dp::GpuProgram> program3d = m_gpuProgramManager->GetProgram(state.GetProgram3dIndex());
  group->SetRenderParams(program, program3d, make_ref(&m_generalUniforms));
  group->AddBucket(move(renderBucket));

  layer.m_renderGroups.push_back(move(group));
  layer.m_isDirty = true;
}

void FrontendRenderer::RemoveRenderGroupsLater(TRenderGroupRemovePredicate const & predicate)
{
  ASSERT(predicate != nullptr, ());

  for (RenderLayer & layer : m_layers)
  {
    RemoveTilePredicate f(predicate);
    RemoveGroups(f, layer.m_renderGroups, make_ref(m_overlayTree));
    layer.m_isDirty |= f.m_deletionMark;
  }
}

bool FrontendRenderer::CheckTileGenerations(TileKey const & tileKey)
{
  bool const result = (tileKey.m_generation >= m_maxGeneration);

  if (tileKey.m_generation > m_maxGeneration)
    m_maxGeneration = tileKey.m_generation;

  auto removePredicate = [&tileKey](drape_ptr<RenderGroup> const & group)
  {
    return group->GetTileKey() == tileKey && group->GetTileKey().m_generation < tileKey.m_generation;
  };
  RemoveRenderGroupsLater(removePredicate);

  return result;
}

void FrontendRenderer::OnCompassTapped()
{
  m_myPositionController->OnCompassTapped();
}

FeatureID FrontendRenderer::GetVisiblePOI(m2::PointD const & pixelPoint)
{
  double halfSize = VisualParams::Instance().GetTouchRectRadius();
  m2::PointD sizePoint(halfSize, halfSize);
  m2::RectD selectRect(pixelPoint - sizePoint, pixelPoint + sizePoint);
  return GetVisiblePOI(selectRect);
}

FeatureID FrontendRenderer::GetVisiblePOI(m2::RectD const & pixelRect)
{
  m2::PointD pt = pixelRect.Center();
  dp::TOverlayContainer selectResult;
  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  if (m_overlayTree->IsNeedUpdate())
    BuildOverlayTree(screen);
  m_overlayTree->Select(pixelRect, selectResult);

  double dist = numeric_limits<double>::max();
  FeatureID featureID;
  for (ref_ptr<dp::OverlayHandle> handle : selectResult)
  {
    double const curDist = pt.SquareLength(handle->GetPivot(screen, screen.isPerspective()));
    if (curDist < dist)
    {
      dist = curDist;
      featureID = handle->GetOverlayID().m_featureId;
    }
  }

  return featureID;
}

void FrontendRenderer::PrepareGpsTrackPoints(uint32_t pointsCount)
{
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                            make_unique_dp<CacheGpsTrackPointsMessage>(pointsCount),
                            MessagePriority::Normal);
}

void FrontendRenderer::PullToBoundArea(bool randomPlace, bool applyZoom)
{
  if (m_dragBoundArea.empty())
    return;

  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  m2::PointD const center = screen.GlobalRect().Center();
  if (!m2::IsPointInsideTriangles(center, m_dragBoundArea))
  {
    m2::PointD const dest = randomPlace ? m2::GetRandomPointInsideTriangles(m_dragBoundArea) :
                                          m2::ProjectPointToTriangles(center, m_dragBoundArea);
    int zoom = kDoNotChangeZoom;
    if (applyZoom && m_currentZoomLevel < scales::GetAddNewPlaceScale())
      zoom = scales::GetAddNewPlaceScale();
    AddUserEvent(make_unique_dp<SetCenterEvent>(dest, zoom, true));
  }
}

void FrontendRenderer::ProcessSelection(ref_ptr<SelectObjectMessage> msg)
{
  if (msg->IsDismiss())
  {
    m_selectionShape->Hide();
  }
  else
  {
    double offsetZ = 0.0;
    if (m_userEventStream.GetCurrentScreen().isPerspective())
    {
      dp::TOverlayContainer selectResult;
      if (m_overlayTree->IsNeedUpdate())
        BuildOverlayTree(m_userEventStream.GetCurrentScreen());
      m_overlayTree->Select(msg->GetPosition(), selectResult);
      for (ref_ptr<dp::OverlayHandle> handle : selectResult)
        offsetZ = max(offsetZ, handle->GetPivotZ());
    }
    m_selectionShape->Show(msg->GetSelectedObject(), msg->GetPosition(), offsetZ, msg->IsAnim());
  }
}

void FrontendRenderer::BeginUpdateOverlayTree(ScreenBase const & modelView)
{
  if (m_overlayTree->Frame())
    m_overlayTree->StartOverlayPlacing(modelView);
}

void FrontendRenderer::UpdateOverlayTree(ScreenBase const & modelView, drape_ptr<RenderGroup> & renderGroup)
{
  if (m_overlayTree->IsNeedUpdate())
    renderGroup->CollectOverlay(make_ref(m_overlayTree));
  else
    renderGroup->Update(modelView);
}

void FrontendRenderer::EndUpdateOverlayTree()
{
  if (m_overlayTree->IsNeedUpdate())
  {
    m_overlayTree->EndOverlayPlacing();

    // Track overlays.
    if (m_overlaysTracker->StartTracking(m_currentZoomLevel,
                                         m_myPositionController->IsModeHasPosition(),
                                         m_myPositionController->GetDrawablePosition(),
                                         m_myPositionController->GetHorizontalAccuracy()))
    {
      for (auto const & handle : m_overlayTree->GetHandlesCache())
      {
        if (handle->IsVisible())
          m_overlaysTracker->Track(handle->GetOverlayID().m_featureId);
      }
      m_overlaysTracker->FinishTracking();
    }
  }
}

void FrontendRenderer::RenderScene(ScreenBase const & modelView)
{
#if defined(DRAPE_MEASURER) && (defined(RENDER_STATISTIC) || defined(TRACK_GPU_MEM))
  DrapeMeasurer::Instance().BeforeRenderFrame();
#endif

  GLFunctions::glEnable(gl_const::GLDepthTest);
  m_viewport.Apply();
  RefreshBgColor();
  GLFunctions::glClear();

  Render2dLayer(modelView);

  if (m_framebuffer->IsSupported())
  {
    RenderTrafficAndRouteLayer(modelView);
    Render3dLayer(modelView, true /* useFramebuffer */);
  }
  else
  {
    Render3dLayer(modelView, false /* useFramebuffer */);
    RenderTrafficAndRouteLayer(modelView);
  }

  // After this line we do not use (almost) depth buffer.
  GLFunctions::glDisable(gl_const::GLDepthTest);
  GLFunctions::glClearDepth();

  if (m_selectionShape != nullptr)
  {
    SelectionShape::ESelectedObject selectedObject = m_selectionShape->GetSelectedObject();
    if (selectedObject == SelectionShape::OBJECT_MY_POSITION)
    {
      ASSERT(m_myPositionController->IsModeHasPosition(), ());
      m_selectionShape->SetPosition(m_myPositionController->Position());
      m_selectionShape->Render(modelView, m_currentZoomLevel,
                               make_ref(m_gpuProgramManager), m_generalUniforms);
    }
    else if (selectedObject == SelectionShape::OBJECT_POI)
    {
      m_selectionShape->Render(modelView, m_currentZoomLevel, make_ref(m_gpuProgramManager), m_generalUniforms);
    }
  }

  RenderOverlayLayer(modelView);

  m_gpsTrackRenderer->RenderTrack(modelView, m_currentZoomLevel, make_ref(m_gpuProgramManager), m_generalUniforms);

  if (m_selectionShape != nullptr && m_selectionShape->GetSelectedObject() == SelectionShape::OBJECT_USER_MARK)
    m_selectionShape->Render(modelView, m_currentZoomLevel, make_ref(m_gpuProgramManager), m_generalUniforms);

  RenderUserMarksLayer(modelView);

  m_routeRenderer->RenderRouteSigns(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  m_myPositionController->Render(modelView, m_currentZoomLevel, make_ref(m_gpuProgramManager), m_generalUniforms);

  m_drapeApiRenderer->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  if (m_guiRenderer != nullptr)
    m_guiRenderer->Render(make_ref(m_gpuProgramManager), m_myPositionController->IsInRouting(), modelView);

#if defined(RENDER_DEBUG_RECTS) && defined(COLLECT_DISPLACEMENT_INFO)
  for (auto const & arrow : m_overlayTree->GetDisplacementInfo())
    dp::DebugRectRenderer::Instance().DrawArrow(modelView, arrow);
#endif

#if defined(DRAPE_MEASURER) && (defined(RENDER_STATISTIC) || defined(TRACK_GPU_MEM))
  DrapeMeasurer::Instance().AfterRenderFrame();
#endif

  MergeBuckets();
}

void FrontendRenderer::Render2dLayer(ScreenBase const & modelView)
{
  RenderLayer & layer2d = m_layers[RenderLayer::Geometry2dID];
  layer2d.Sort(make_ref(m_overlayTree));

  for (drape_ptr<RenderGroup> const & group : layer2d.m_renderGroups)
    RenderSingleGroup(modelView, make_ref(group));
}

void FrontendRenderer::Render3dLayer(ScreenBase const & modelView, bool useFramebuffer)
{
  float const kOpacity = 0.7f;
  if (useFramebuffer)
  {
    ASSERT(m_framebuffer->IsSupported(), ());
    m_framebuffer->Enable();
    GLFunctions::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GLFunctions::glClear();
  }

  GLFunctions::glClearDepth();
  GLFunctions::glEnable(gl_const::GLDepthTest);
  RenderLayer & layer = m_layers[RenderLayer::Geometry3dID];
  layer.Sort(make_ref(m_overlayTree));
  for (drape_ptr<RenderGroup> const & group : layer.m_renderGroups)
    RenderSingleGroup(modelView, make_ref(group));

  if (useFramebuffer)
  {
    m_framebuffer->Disable();
    GLFunctions::glDisable(gl_const::GLDepthTest);
    m_screenQuadRenderer->RenderTexture(m_framebuffer->GetTextureId(),
                                        make_ref(m_gpuProgramManager), kOpacity);
  }
}

void FrontendRenderer::RenderOverlayLayer(ScreenBase const & modelView)
{
  RenderLayer & overlay = m_layers[RenderLayer::OverlayID];
  BuildOverlayTree(modelView);
  for (drape_ptr<RenderGroup> & group : overlay.m_renderGroups)
    RenderSingleGroup(modelView, make_ref(group));
}

void FrontendRenderer::RenderTrafficAndRouteLayer(ScreenBase const & modelView)
{
  GLFunctions::glClearDepth();
  GLFunctions::glEnable(gl_const::GLDepthTest);
  if (m_trafficRenderer->HasRenderData())
  {
    m_trafficRenderer->RenderTraffic(modelView, m_currentZoomLevel, 1.0f /* opacity */,
                                     make_ref(m_gpuProgramManager), m_generalUniforms);
  }
  GLFunctions::glClearDepth();
  m_routeRenderer->RenderRoute(modelView, m_trafficRenderer->HasRenderData(),
                               make_ref(m_gpuProgramManager), m_generalUniforms);
}

void FrontendRenderer::RenderUserMarksLayer(ScreenBase const & modelView)
{
  double const kExtension = 1.1;
  m2::RectD screenRect = modelView.ClipRect();
  screenRect.Scale(kExtension);
  for (auto const & group : m_userMarkRenderGroups)
  {
    ASSERT(group.get() != nullptr, ());
    if (m_userMarkVisibility.find(group->GetLayerId()) != m_userMarkVisibility.end())
    {
      if (!group->CanBeClipped() || screenRect.IsIntersect(group->GetTileKey().GetGlobalRect()))
        RenderSingleGroup(modelView, make_ref(group));
    }
  }
}

void FrontendRenderer::BuildOverlayTree(ScreenBase const & modelView)
{
  RenderLayer & overlay = m_layers[RenderLayer::OverlayID];
  overlay.Sort(make_ref(m_overlayTree));
  BeginUpdateOverlayTree(modelView);
  for (drape_ptr<RenderGroup> & group : overlay.m_renderGroups)
    UpdateOverlayTree(modelView, group);
  EndUpdateOverlayTree();
}

void FrontendRenderer::PrepareBucket(dp::GLState const & state, drape_ptr<dp::RenderBucket> & bucket)
{
  ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
  ref_ptr<dp::GpuProgram> program3d = m_gpuProgramManager->GetProgram(state.GetProgram3dIndex());
  bool const isPerspective = m_userEventStream.GetCurrentScreen().isPerspective();
  if (isPerspective)
    program3d->Bind();
  else
    program->Bind();
  bucket->GetBuffer()->Build(isPerspective ? program3d : program);
}

void FrontendRenderer::MergeBuckets()
{
  if (BatchMergeHelper::IsMergeSupported() == false)
    return;

  ++m_mergeBucketsCounter;
  if (m_mergeBucketsCounter < 60)
    return;

  m_mergeBucketsCounter = 0;

  auto mergeFn = [](RenderLayer & layer, bool isPerspective)
  {
    if (layer.m_renderGroups.empty())
      return;

    using TGroupMap = map<MergedGroupKey, vector<drape_ptr<RenderGroup>>>;
    TGroupMap forMerge;

    vector<drape_ptr<RenderGroup>> newGroups;
    newGroups.reserve(layer.m_renderGroups.size());

    size_t groupsCount = layer.m_renderGroups.size();
    for (size_t i = 0; i < groupsCount; ++i)
    {
      ref_ptr<RenderGroup> group = make_ref(layer.m_renderGroups[i]);
      if (!group->IsPendingOnDelete())
      {
        dp::GLState state = group->GetState();
        ASSERT_EQUAL(state.GetDepthLayer(), dp::GLState::GeometryLayer, ());
        forMerge[MergedGroupKey(state, group->GetTileKey())].push_back(move(layer.m_renderGroups[i]));
      }
      else
      {
        newGroups.push_back(move(layer.m_renderGroups[i]));
      }
    }

    for (TGroupMap::value_type & node : forMerge)
    {
      if (node.second.size() < 2)
        newGroups.emplace_back(move(node.second.front()));
      else
        BatchMergeHelper::MergeBatches(node.second, newGroups, isPerspective);
    }

    layer.m_renderGroups = move(newGroups);
    layer.m_isDirty = true;
  };

  bool const isPerspective = m_userEventStream.GetCurrentScreen().isPerspective();
  mergeFn(m_layers[RenderLayer::Geometry2dID], isPerspective);
  mergeFn(m_layers[RenderLayer::Geometry3dID], isPerspective);
}

bool FrontendRenderer::IsPerspective() const
{
  return m_userEventStream.GetCurrentScreen().isPerspective();
}

void FrontendRenderer::RenderSingleGroup(ScreenBase const & modelView, ref_ptr<BaseRenderGroup> group)
{
  group->UpdateAnimation();
  group->Render(modelView);
}

void FrontendRenderer::RefreshProjection(ScreenBase const & screen)
{
  array<float, 16> m;

  dp::MakeProjection(m, 0.0f, screen.GetWidth(), screen.GetHeight(), 0.0f);
  m_generalUniforms.SetMatrix4x4Value("projection", m.data());
}

void FrontendRenderer::RefreshZScale(ScreenBase const & screen)
{
  m_generalUniforms.SetFloatValue("zScale", screen.GetZScale());
}

void FrontendRenderer::RefreshPivotTransform(ScreenBase const & screen)
{
  if (screen.isPerspective())
  {
    math::Matrix<float, 4, 4> transform(screen.Pto3dMatrix());
    m_generalUniforms.SetMatrix4x4Value("pivotTransform", transform.m_data);
  }
  else if (m_isIsometry)
  {
    math::Matrix<float, 4, 4> transform(math::Identity<float, 4>());
    transform(2, 1) = -1.0f / tan(kIsometryAngle);
    transform(2, 2) = 1.0f / screen.GetHeight();
    m_generalUniforms.SetMatrix4x4Value("pivotTransform", transform.m_data);
  }
  else
  {
    math::Matrix<float, 4, 4> transform(math::Identity<float, 4>());
    m_generalUniforms.SetMatrix4x4Value("pivotTransform", transform.m_data);
  }
}

void FrontendRenderer::RefreshBgColor()
{
  uint32_t color = drule::rules().GetBgColor(df::GetDrawTileScale(m_userEventStream.GetCurrentScreen()));
  dp::Color c = dp::Extract(color, 0 /*255 - (color >> 24)*/);
  GLFunctions::glClearColor(c.GetRedF(), c.GetGreenF(), c.GetBlueF(), 1.0f);
}

void FrontendRenderer::DisablePerspective()
{
  AddUserEvent(make_unique_dp<SetAutoPerspectiveEvent>(false /* isAutoPerspective */));
}

void FrontendRenderer::CheckIsometryMinScale(ScreenBase const & screen)
{
  bool const isScaleAllowableIn3d = IsScaleAllowableIn3d(m_currentZoomLevel);
  bool const isIsometry = m_enable3dBuildings && !m_choosePositionMode && isScaleAllowableIn3d;
  if (m_isIsometry != isIsometry)
  {
    m_isIsometry = isIsometry;
    RefreshPivotTransform(screen);
  }
}

void FrontendRenderer::ResolveZoomLevel(ScreenBase const & screen)
{
  int const prevZoomLevel = m_currentZoomLevel;
  m_currentZoomLevel = GetDrawTileScale(screen);

  if (prevZoomLevel != m_currentZoomLevel)
    UpdateCanBeDeletedStatus();

  CheckIsometryMinScale(screen);
  UpdateDisplacementEnabled();
}

void FrontendRenderer::UpdateDisplacementEnabled()
{
  if (m_choosePositionMode)
    m_overlayTree->SetDisplacementEnabled(m_currentZoomLevel < scales::GetAddNewPlaceScale());
  else
    m_overlayTree->SetDisplacementEnabled(true);
}

void FrontendRenderer::OnTap(m2::PointD const & pt, bool isLongTap)
{
  if (m_blockTapEvents)
    return;

  double halfSize = VisualParams::Instance().GetTouchRectRadius();
  m2::PointD sizePoint(halfSize, halfSize);
  m2::RectD selectRect(pt - sizePoint, pt + sizePoint);

  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  bool isMyPosition = false;
  if (m_myPositionController->IsModeHasPosition())
  {
    m2::PointD const pt = screen.PtoP3d(screen.GtoP(m_myPositionController->Position()));
    isMyPosition = selectRect.IsPointInside(pt);
  }

  m_tapEventInfoFn({pt, isLongTap, isMyPosition, GetVisiblePOI(selectRect)});
}

void FrontendRenderer::OnForceTap(m2::PointD const & pt)
{
  // Emulate long tap on force tap.
  OnTap(pt, true /* isLongTap */);
}

void FrontendRenderer::OnDoubleTap(m2::PointD const & pt)
{
  m_userEventStream.AddEvent(make_unique_dp<ScaleEvent>(2.0 /* scale factor */, pt, true /* animated */));
}

void FrontendRenderer::OnTwoFingersTap()
{
  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  m_userEventStream.AddEvent(make_unique_dp<ScaleEvent>(0.5 /* scale factor */, screen.PixelRect().Center(),
                                                        true /* animated */));
}

bool FrontendRenderer::OnSingleTouchFiltrate(m2::PointD const & pt, TouchEvent::ETouchType type)
{
  // This method can be called before gui rendererer initialization.
  if (m_guiRenderer == nullptr)
    return false;

  float const rectHalfSize = df::VisualParams::Instance().GetTouchRectRadius();
  m2::RectD r(-rectHalfSize, -rectHalfSize, rectHalfSize, rectHalfSize);
  r.SetCenter(pt);

  switch(type)
  {
  case TouchEvent::ETouchType::TOUCH_DOWN:
    return m_guiRenderer->OnTouchDown(r);
  case TouchEvent::ETouchType::TOUCH_UP:
    m_guiRenderer->OnTouchUp(r);
    return false;
  case TouchEvent::ETouchType::TOUCH_CANCEL:
    m_guiRenderer->OnTouchCancel(r);
    return false;
  case TouchEvent::ETouchType::TOUCH_MOVE:
    return false;
  }

  return false;
}

void FrontendRenderer::OnDragStarted()
{
  m_myPositionController->DragStarted();
}

void FrontendRenderer::OnDragEnded(m2::PointD const & distance)
{
  m_myPositionController->DragEnded(distance);
  PullToBoundArea(false /* randomPlace */, false /* applyZoom */);
}

void FrontendRenderer::OnScaleStarted()
{
  m_myPositionController->ScaleStarted();
}

void FrontendRenderer::OnRotated()
{
  m_myPositionController->Rotated();
}

void FrontendRenderer::CorrectScalePoint(m2::PointD & pt) const
{
  m_myPositionController->CorrectScalePoint(pt);
}

void FrontendRenderer::CorrectGlobalScalePoint(m2::PointD & pt) const
{
  m_myPositionController->CorrectGlobalScalePoint(pt);
}

void FrontendRenderer::CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const
{
  m_myPositionController->CorrectScalePoint(pt1, pt2);
}

void FrontendRenderer::OnScaleEnded()
{
  m_myPositionController->ScaleEnded();
  PullToBoundArea(false /* randomPlace */, false /* applyZoom */);
}

void FrontendRenderer::OnAnimatedScaleEnded()
{
  m_myPositionController->ResetBlockAutoZoomTimer();
  PullToBoundArea(false /* randomPlace */, false /* applyZoom */);
}

void FrontendRenderer::OnTouchMapAction()
{
  m_myPositionController->ResetRoutingNotFollowTimer();
}

bool FrontendRenderer::OnNewVisibleViewport(m2::RectD const & oldViewport, m2::RectD const & newViewport, m2::PointD & gOffset)
{
  gOffset = m2::PointD(0, 0);
  if (m_myPositionController->IsModeChangeViewport() || m_selectionShape == nullptr || oldViewport == newViewport)
    return false;

  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();

  ScreenBase targetScreen;
  AnimationSystem::Instance().GetTargetScreen(screen, targetScreen);

  m2::PointD pos;
  m2::PointD targetPos;
  if (m_selectionShape->IsVisible(screen, pos) &&
      m_selectionShape->IsVisible(targetScreen, targetPos))
  {
    m2::RectD rect(pos, pos);
    m2::RectD targetRect(targetPos, targetPos);

    if (m_overlayTree->IsNeedUpdate())
      BuildOverlayTree(screen);

    if (!(m_selectionShape->GetSelectedObject() == SelectionShape::OBJECT_POI &&
          m_overlayTree->GetSelectedFeatureRect(screen, rect) &&
          m_overlayTree->GetSelectedFeatureRect(targetScreen, targetRect)))
    {
      double const r = m_selectionShape->GetRadius();
      rect.Inflate(r, r);
      targetRect.Inflate(r, r);
    }

    if (oldViewport.IsIntersect(targetRect) && !newViewport.IsRectInside(rect))
    {
      double const kOffset = 50 * VisualParams::Instance().GetVisualScale();
      m2::PointD pOffset(0.0, 0.0);
      if (rect.minX() < newViewport.minX())
        pOffset.x = newViewport.minX() - rect.minX() + kOffset;
      else if (rect.maxX() > newViewport.maxX())
        pOffset.x = newViewport.maxX() - rect.maxX() - kOffset;

      if (rect.minY() < newViewport.minY())
        pOffset.y = newViewport.minY() - rect.minY() + kOffset;
      else if (rect.maxY() > newViewport.maxY())
        pOffset.y = newViewport.maxY() - rect.maxY() - kOffset;

      gOffset = screen.PtoG(screen.P3dtoP(pos + pOffset)) - screen.PtoG(screen.P3dtoP(pos));
      return true;
    }
  }
  return false;
}

TTilesCollection FrontendRenderer::ResolveTileKeys(ScreenBase const & screen)
{
  m2::RectD const & rect = screen.ClipRect();
  int const dataZoomLevel = ClipTileZoomByMaxDataZoom(m_currentZoomLevel);

  m_notFinishedTiles.clear();

  // Request new tiles.
  TTilesCollection tiles;
  buffer_vector<TileKey, 8> tilesToDelete;
  CoverageResult result = CalcTilesCoverage(rect, dataZoomLevel,
                                            [this, &rect, &tiles, &tilesToDelete](int tileX, int tileY)
  {
    TileKey const key(tileX, tileY, m_currentZoomLevel);
    if (rect.IsIntersect(key.GetGlobalRect()))
    {
      tiles.insert(key);
      m_notFinishedTiles.insert(key);
    }
    else
    {
      tilesToDelete.push_back(key);
    }
  });

  // Remove old tiles.
  auto removePredicate = [this, &result, &tilesToDelete](drape_ptr<RenderGroup> const & group)
  {
    TileKey const & key = group->GetTileKey();
    return group->GetTileKey().m_zoomLevel == m_currentZoomLevel &&
           (key.m_x < result.m_minTileX || key.m_x >= result.m_maxTileX ||
           key.m_y < result.m_minTileY || key.m_y >= result.m_maxTileY ||
           find(tilesToDelete.begin(), tilesToDelete.end(), key) != tilesToDelete.end());
  };
  for (RenderLayer & layer : m_layers)
    layer.m_isDirty |= RemoveGroups(removePredicate, layer.m_renderGroups, make_ref(m_overlayTree));

  RemoveRenderGroupsLater([this](drape_ptr<RenderGroup> const & group)
  {
    return group->GetTileKey().m_zoomLevel != m_currentZoomLevel;
  });

  m_trafficRenderer->OnUpdateViewport(result, m_currentZoomLevel, tilesToDelete);

#if defined(DRAPE_MEASURER) && defined(GENERATING_STATISTIC)
  DrapeMeasurer::Instance().StartScenePreparing();
#endif

  return tiles;
}

void FrontendRenderer::OnContextDestroy()
{
  LOG(LINFO, ("On context destroy."));

  // Clear all graphics.
  for (RenderLayer & layer : m_layers)
  {
    layer.m_renderGroups.clear();
    layer.m_isDirty = false;
  }

  m_selectObjectMessage.reset();
  m_overlayTree->SetSelectedFeature(FeatureID());

  m_userMarkRenderGroups.clear();
  m_guiRenderer.reset();
  m_selectionShape.reset();
  m_framebuffer.reset();
  m_screenQuadRenderer.reset();

  m_myPositionController->ResetRenderShape();
  m_routeRenderer->ClearGLDependentResources();
  m_gpsTrackRenderer->ClearRenderData();
  m_trafficRenderer->ClearGLDependentResources();
  m_drapeApiRenderer->Clear();

#ifdef RENDER_DEBUG_RECTS
  dp::DebugRectRenderer::Instance().Destroy();
#endif

  m_gpuProgramManager.reset();
  m_contextFactory->getDrawContext()->doneCurrent();

  m_needRestoreSize = true;
}

void FrontendRenderer::OnContextCreate()
{
  LOG(LINFO, ("On context create."));

  m_contextFactory->waitForInitialization();

  dp::OGLContext * context = m_contextFactory->getDrawContext();
  context->makeCurrent();

  GLFunctions::Init(m_apiVersion);
  GLFunctions::AttachCache(this_thread::get_id());

  GLFunctions::glPixelStore(gl_const::GLUnpackAlignment, 1);

  GLFunctions::glClearDepthValue(1.0);
  GLFunctions::glDepthFunc(gl_const::GLLessOrEqual);
  GLFunctions::glDepthMask(true);

  GLFunctions::glFrontFace(gl_const::GLClockwise);
  GLFunctions::glCullFace(gl_const::GLBack);
  GLFunctions::glEnable(gl_const::GLCullFace);
  GLFunctions::glEnable(gl_const::GLScissorTest);

  dp::SupportManager::Instance().Init();

  m_gpuProgramManager = make_unique_dp<dp::GpuProgramManager>();
  m_gpuProgramManager->Init(make_unique_dp<gpu::ShaderMapper>(m_apiVersion));

  dp::BlendingParams blendingParams;
  blendingParams.Apply();

#ifdef RENDER_DEBUG_RECTS
  dp::DebugRectRenderer::Instance().Init(make_ref(m_gpuProgramManager), gpu::DEBUG_RECT_PROGRAM);
#endif

  // resources recovering
  m_framebuffer.reset(new dp::Framebuffer());
  m_framebuffer->SetDefaultContext(context);

  m_screenQuadRenderer.reset(new ScreenQuadRenderer());
}

FrontendRenderer::Routine::Routine(FrontendRenderer & renderer) : m_renderer(renderer) {}

void FrontendRenderer::Routine::Do()
{
  LOG(LINFO, ("Start routine."));

  gui::DrapeGui::Instance().ConnectOnCompassTappedHandler(bind(&FrontendRenderer::OnCompassTapped, &m_renderer));
  m_renderer.m_myPositionController->SetListener(ref_ptr<MyPositionController::Listener>(&m_renderer));
  m_renderer.m_userEventStream.SetListener(ref_ptr<UserEventStream::Listener>(&m_renderer));

  m_renderer.OnContextCreate();

  double const kMaxInactiveSeconds = 2.0;
  double const kShowOverlaysEventsPeriod = 5.0;

  my::Timer timer;
  my::Timer activityTimer;
  my::Timer showOverlaysEventsTimer;

  double frameTime = 0.0;
  bool modelViewChanged = true;
  bool viewportChanged = true;

  dp::OGLContext * context = m_renderer.m_contextFactory->getDrawContext();

  while (!IsCancelled())
  {
    ScreenBase modelView = m_renderer.ProcessEvents(modelViewChanged, viewportChanged);
    if (viewportChanged)
      m_renderer.OnResize(modelView);

    context->setDefaultFramebuffer();

    if (modelViewChanged || viewportChanged)
      m_renderer.PrepareScene(modelView);

    // Check for a frame is active.
    bool isActiveFrame = modelViewChanged || viewportChanged ||
                         m_renderer.m_myPositionController->IsWaitingForTimers();

    isActiveFrame |= m_renderer.m_texMng->UpdateDynamicTextures();
    m_renderer.RenderScene(modelView);

    if (modelViewChanged || m_renderer.m_forceUpdateScene)
      m_renderer.UpdateScene(modelView);

    isActiveFrame |= InterpolationHolder::Instance().Advance(frameTime);
    AnimationSystem::Instance().Advance(frameTime);

    isActiveFrame |= m_renderer.m_userEventStream.IsWaitingForActionCompletion();

    if (isActiveFrame)
      activityTimer.Reset();

    bool isValidFrameTime = true;
    if (activityTimer.ElapsedSeconds() > kMaxInactiveSeconds)
    {
      // Process a message or wait for a message.
      // IsRenderingEnabled() can return false in case of rendering disabling and we must prevent
      // possibility of infinity waiting in ProcessSingleMessage.
      m_renderer.ProcessSingleMessage(m_renderer.IsRenderingEnabled());
      activityTimer.Reset();
      timer.Reset();
      isValidFrameTime = false;
    }
    else
    {
      double availableTime = kVSyncInterval - timer.ElapsedSeconds();
      do
      {
        if (!m_renderer.ProcessSingleMessage(false /* waitForMessage */))
          break;

        activityTimer.Reset();
        availableTime = kVSyncInterval - timer.ElapsedSeconds();
      }
      while (availableTime > 0);
    }

    context->present();
    frameTime = timer.ElapsedSeconds();
    timer.Reset();

    // Limit fps in following mode.
    double constexpr kFrameTime = 1.0 / 30.0;
    if (isValidFrameTime &&
        m_renderer.m_myPositionController->IsRouteFollowingActive() && frameTime < kFrameTime)
    {
      uint32_t const ms = static_cast<uint32_t>((kFrameTime - frameTime) * 1000);
      this_thread::sleep_for(milliseconds(ms));
    }

    if (m_renderer.m_overlaysTracker->IsValid() &&
        showOverlaysEventsTimer.ElapsedSeconds() > kShowOverlaysEventsPeriod)
    {
      m_renderer.CollectShowOverlaysEvents();
      showOverlaysEventsTimer.Reset();
    }

    m_renderer.CheckRenderingEnabled();
  }

  m_renderer.CollectShowOverlaysEvents();

#ifdef RENDER_DEBUG_RECTS
  dp::DebugRectRenderer::Instance().Destroy();
#endif

  m_renderer.ReleaseResources();
}

void FrontendRenderer::ReleaseResources()
{
  for (RenderLayer & layer : m_layers)
    layer.m_renderGroups.clear();

  m_userMarkRenderGroups.clear();
  m_guiRenderer.reset();
  m_myPositionController.reset();
  m_selectionShape.release();
  m_routeRenderer.reset();
  m_framebuffer.reset();
  m_screenQuadRenderer.reset();
  m_trafficRenderer.reset();

  m_gpuProgramManager.reset();
  m_contextFactory->getDrawContext()->doneCurrent();
}

void FrontendRenderer::AddUserEvent(drape_ptr<UserEvent> && event)
{
#ifdef SCENARIO_ENABLE
  if (m_scenarioManager->IsRunning() && event->GetType() == UserEvent::EventType::Touch)
    return;
#endif
  m_userEventStream.AddEvent(move(event));
  if (IsInInfinityWaiting())
    CancelMessageWaiting();
}

void FrontendRenderer::PositionChanged(m2::PointD const & position)
{
  m_userPositionChangedFn(position);
}

void FrontendRenderer::ChangeModelView(m2::PointD const & center, int zoomLevel, TAnimationCreator const & parallelAnimCreator)
{
  AddUserEvent(make_unique_dp<SetCenterEvent>(center, zoomLevel, true, parallelAnimCreator));
}

void FrontendRenderer::ChangeModelView(double azimuth, TAnimationCreator const & parallelAnimCreator)
{
  AddUserEvent(make_unique_dp<RotateEvent>(azimuth, parallelAnimCreator));
}

void FrontendRenderer::ChangeModelView(m2::RectD const & rect, TAnimationCreator const & parallelAnimCreator)
{
  AddUserEvent(make_unique_dp<SetRectEvent>(rect, true, kDoNotChangeZoom, true, parallelAnimCreator));
}

void FrontendRenderer::ChangeModelView(m2::PointD const & userPos, double azimuth,
                                       m2::PointD const & pxZero, int preferredZoomLevel,
                                       TAnimationCreator const & parallelAnimCreator)
{
  AddUserEvent(make_unique_dp<FollowAndRotateEvent>(userPos, pxZero, azimuth, preferredZoomLevel, true, parallelAnimCreator));
}

void FrontendRenderer::ChangeModelView(double autoScale, m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero,
                                       TAnimationCreator const & parallelAnimCreator)
{
  AddUserEvent(make_unique_dp<FollowAndRotateEvent>(userPos, pxZero, azimuth, autoScale, parallelAnimCreator));
}

ScreenBase const & FrontendRenderer::ProcessEvents(bool & modelViewChanged, bool & viewportChanged)
{
  ScreenBase const & modelView = m_userEventStream.ProcessEvents(modelViewChanged, viewportChanged);
  gui::DrapeGui::Instance().SetInUserAction(m_userEventStream.IsInUserAction());

  return modelView;
}

void FrontendRenderer::PrepareScene(ScreenBase const & modelView)
{
  RefreshZScale(modelView);
  RefreshPivotTransform(modelView);

  m_myPositionController->OnUpdateScreen(modelView);
  m_routeRenderer->UpdateRoute(modelView, bind(&FrontendRenderer::OnCacheRouteArrows, this, _1, _2));
}

void FrontendRenderer::UpdateScene(ScreenBase const & modelView)
{
  ResolveZoomLevel(modelView);

  m_gpsTrackRenderer->Update();

  auto removePredicate = [this](drape_ptr<RenderGroup> const & group)
  {
    uint32_t const kMaxGenerationRange = 5;
    TileKey const & key = group->GetTileKey();
    return (group->IsOverlay() && key.m_zoomLevel > m_currentZoomLevel) ||
            (m_maxGeneration - key.m_generation > kMaxGenerationRange);
  };
  for (RenderLayer & layer : m_layers)
    layer.m_isDirty |= RemoveGroups(removePredicate, layer.m_renderGroups, make_ref(m_overlayTree));

  if (m_forceUpdateScene || m_lastReadedModelView != modelView)
  {
    EmitModelViewChanged(modelView);
    m_lastReadedModelView = modelView;
    m_requestedTiles->Set(modelView, m_isIsometry || modelView.isPerspective(), m_forceUpdateScene,
                          ResolveTileKeys(modelView));
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<UpdateReadManagerMessage>(),
                              MessagePriority::UberHighSingleton);
    m_forceUpdateScene = false;
  }
}

void FrontendRenderer::EmitModelViewChanged(ScreenBase const & modelView) const
{
  m_modelViewChangedFn(modelView);
}

void FrontendRenderer::OnCacheRouteArrows(int routeIndex, vector<ArrowBorders> const & borders)
{
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                            make_unique_dp<CacheRouteArrowsMessage>(routeIndex, borders),
                            MessagePriority::Normal);
}

drape_ptr<ScenarioManager> const & FrontendRenderer::GetScenarioManager() const
{
  return m_scenarioManager;
}

void FrontendRenderer::CollectShowOverlaysEvents()
{
  ASSERT(m_overlaysShowStatsCallback != nullptr, ());
  m_overlaysShowStatsCallback(m_overlaysTracker->Collect());
}

FrontendRenderer::RenderLayer::RenderLayerID FrontendRenderer::RenderLayer::GetLayerID(dp::GLState const & state)
{
  if (state.GetDepthLayer() == dp::GLState::OverlayLayer)
    return OverlayID;

  if (state.GetProgram3dIndex() == gpu::AREA_3D_PROGRAM || state.GetProgram3dIndex() == gpu::AREA_3D_OUTLINE_PROGRAM)
    return Geometry3dID;

  return Geometry2dID;
}

void FrontendRenderer::RenderLayer::Sort(ref_ptr<dp::OverlayTree> overlayTree)
{
  if (!m_isDirty)
    return;

  RenderGroupComparator comparator;
  sort(m_renderGroups.begin(), m_renderGroups.end(), ref(comparator));
  m_isDirty = comparator.m_pendingOnDeleteFound;

  while (!m_renderGroups.empty() && m_renderGroups.back()->CanBeDeleted())
  {
    m_renderGroups.back()->RemoveOverlay(overlayTree);
    m_renderGroups.pop_back();
  }
}

} // namespace df
