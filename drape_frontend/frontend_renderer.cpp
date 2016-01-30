#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/framebuffer.hpp"
#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/transparent_layer.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/user_mark_shapes.hpp"
#include "drape_frontend/batch_merge_helper.hpp"

#include "drape/debug_rect_renderer.hpp"
#include "drape/shader_def.hpp"
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

namespace df
{

namespace
{
constexpr float kIsometryAngle = math::pi * 80.0f / 180.0f;
const double VSyncInterval = 0.06;
//const double VSyncInterval = 0.014;

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
bool RemoveGroups(ToDo & filter, vector<drape_ptr<RenderGroup>> & groups)
{
  size_t startCount = groups.size();
  size_t count = startCount;
  size_t current = 0;
  while (current < count)
  {
    drape_ptr<RenderGroup> & group = groups[current];
    if (filter(move(group)))
    {
      swap(group, groups.back());
      groups.pop_back();
      --count;
    }
    else
      ++current;
  }

  return startCount != count;
}

struct ActivateTilePredicate
{
  TileKey const & m_tileKey;

  ActivateTilePredicate(TileKey const & tileKey)
    : m_tileKey(tileKey)
  {
  }

  bool operator()(drape_ptr<RenderGroup> & group) const
  {
    if (group->GetTileKey() == m_tileKey)
    {
      group->Appear();
      return true;
    }

    return false;
  }
};

struct RemoveTilePredicate
{
  mutable bool m_deletionMark = false;
  function<bool(drape_ptr<RenderGroup> const &)> const & m_predicate;

  RemoveTilePredicate(function<bool(drape_ptr<RenderGroup> const &)> const & predicate)
    : m_predicate(predicate)
  {
  }

  bool operator()(drape_ptr<RenderGroup> && group) const
  {
    if (m_predicate(group))
    {
      group->Disappear();
      group->DeleteLater();
      m_deletionMark = true;
      return group->CanBeDeleted();
    }

    return false;
  }
};

template <typename TPredicate>
struct MoveTileFunctor
{
  TPredicate m_predicate;
  vector<drape_ptr<RenderGroup>> & m_targetGroups;

  MoveTileFunctor(TPredicate predicate, vector<drape_ptr<RenderGroup>> & groups)
    : m_predicate(predicate)
    , m_targetGroups(groups)
  {
  }

  bool operator()(drape_ptr<RenderGroup> && group)
  {
    if (m_predicate(group))
    {
      m_targetGroups.push_back(move(group));
      return true;
    }

    return false;
  }
};

} // namespace

FrontendRenderer::FrontendRenderer(Params const & params)
  : BaseRenderer(ThreadsCommutator::RenderThread, params)
  , m_gpuProgramManager(new dp::GpuProgramManager())
  , m_routeRenderer(new RouteRenderer())
  , m_framebuffer(new Framebuffer())
  , m_transparentLayer(new TransparentLayer())
  , m_gpsTrackRenderer(new GpsTrackRenderer(bind(&FrontendRenderer::PrepareGpsTrackPoints, this, _1)))
  , m_overlayTree(new dp::OverlayTree())
  , m_enablePerspectiveInNavigation(false)
  , m_enable3dBuildings(params.m_allow3dBuildings)
  , m_isIsometry(false)
  , m_viewport(params.m_viewport)
  , m_userEventStream(params.m_isCountryLoadedFn)
  , m_modelViewChangedFn(params.m_modelViewChangedFn)
  , m_tapEventInfoFn(params.m_tapEventFn)
  , m_userPositionChangedFn(params.m_positionChangedFn)
  , m_tileTree(new TileTree())
  , m_requestedTiles(params.m_requestedTiles)
  , m_maxGeneration(0)
{
#ifdef DRAW_INFO
  m_tpf = 0.0;
  m_fps = 0.0;
#endif

#ifdef DEBUG
  m_isTeardowned = false;
#endif

  ASSERT(m_tapEventInfoFn, ());
  ASSERT(m_userPositionChangedFn, ());

  m_myPositionController.reset(new MyPositionController(params.m_initMyPositionMode));
  m_myPositionController->SetModeListener(params.m_myPositionModeCallback);

  StartThread();
}

FrontendRenderer::~FrontendRenderer()
{
  ASSERT(m_isTeardowned, ());
}

void FrontendRenderer::Teardown()
{
  StopThread();
#ifdef DEBUG
  m_isTeardowned = true;
#endif
}

#ifdef DRAW_INFO
void FrontendRenderer::BeforeDrawFrame()
{
  m_frameStartTime = m_timer.ElapsedSeconds();
}

void FrontendRenderer::AfterDrawFrame()
{
  m_drawedFrames++;

  double elapsed = m_timer.ElapsedSeconds();
  m_tpfs.push_back(elapsed - m_frameStartTime);

  if (elapsed > 1.0)
  {
    m_timer.Reset();
    m_fps = m_drawedFrames / elapsed;
    m_drawedFrames = 0;

    m_tpf = accumulate(m_tpfs.begin(), m_tpfs.end(), 0.0) / m_tpfs.size();

    LOG(LINFO, ("Average Fps : ", m_fps));
    LOG(LINFO, ("Average Tpf : ", m_tpf));

#if defined(TRACK_GPU_MEM)
    string report = dp::GPUMemTracker::Inst().Report();
    LOG(LINFO, (report));
#endif
#if defined(TRACK_GLYPH_USAGE)
    string glyphReport = dp::GlyphUsageTracker::Instance().Report();
    LOG(LINFO, (glyphReport));
#endif
  }
}

#endif

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
      ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
      ref_ptr<dp::GpuProgram> program3d = m_gpuProgramManager->GetProgram(state.GetProgram3dIndex());
      bool const isPerspective = m_userEventStream.GetCurrentScreen().isPerspective();
      if (isPerspective)
        program3d->Bind();
      else
        program->Bind();
      bucket->GetBuffer()->Build(isPerspective ? program3d : program);
      if (!IsUserMarkLayer(key))
      {
        if (CheckTileGenerations(key))
          m_tileTree->ProcessTile(key, GetCurrentZoomLevelForData(), state, move(bucket));
      }
      else
      {
        m_userMarkRenderGroups.emplace_back(make_unique_dp<UserMarkRenderGroup>(state, key, move(bucket)));
        m_userMarkRenderGroups.back()->SetRenderParams(program, program3d, make_ref(&m_generalUniforms));
      }
      break;
    }

  case Message::FinishReading:
    {
      ref_ptr<FinishReadingMessage> msg = message;
      for (auto const & tileKey : msg->GetTiles())
        CheckTileGenerations(tileKey);
      m_tileTree->FinishTiles(msg->GetTiles(), GetCurrentZoomLevelForData());
      break;
    }

  case Message::InvalidateRect:
    {
      ref_ptr<InvalidateRectMessage> m = message;
      InvalidateRect(m->GetRect());
      break;
    }

  case Message::ClearUserMarkLayer:
    {
      TileKey const & tileKey = ref_ptr<ClearUserMarkLayerMessage>(message)->GetKey();
      auto const functor = [&tileKey](drape_ptr<UserMarkRenderGroup> const & g)
      {
        return g->GetTileKey() == tileKey;
      };

      auto const iter = remove_if(m_userMarkRenderGroups.begin(),
                                  m_userMarkRenderGroups.end(),
                                  functor);

      m_userMarkRenderGroups.erase(iter, m_userMarkRenderGroups.end());
      break;
    }

  case Message::ChangeUserMarkLayerVisibility:
    {
      ref_ptr<ChangeUserMarkLayerVisibilityMessage> m = message;
      TileKey const & key = m->GetKey();
      if (m->IsVisible())
        m_userMarkVisibility.insert(key);
      else
        m_userMarkVisibility.erase(key);
      break;
    }

  case Message::GuiLayerRecached:
    {
      ref_ptr<GuiLayerRecachedMessage> msg = message;
      drape_ptr<gui::LayerRenderer> renderer = move(msg->AcceptRenderer());
      renderer->Build(make_ref(m_gpuProgramManager));
      if (m_guiRenderer == nullptr)
        m_guiRenderer = move(renderer);
      else
        m_guiRenderer->Merge(make_ref(renderer));
      break;
    }

  case Message::GuiLayerLayout:
    {
      ASSERT(m_guiRenderer != nullptr, ());
      m_guiRenderer->SetLayout(ref_ptr<GuiLayerLayoutMessage>(message)->GetLayoutInfo());
      break;
    }

  case Message::StopRendering:
    {
      ProcessStopRenderingMessage();
      break;
    }

  case Message::MyPositionShape:
    {
      ref_ptr<MyPositionShapeMessage> msg = message;
      m_myPositionController->SetRenderShape(msg->AcceptShape());
      m_selectionShape = msg->AcceptSelection();
    }
    break;

  case Message::ChangeMyPostitionMode:
    {
      ref_ptr<ChangeMyPositionModeMessage> msg = message;
      switch (msg->GetChangeType())
      {
      case ChangeMyPositionModeMessage::TYPE_NEXT:
        m_myPositionController->NextMode(msg->GetPreferredZoomLevel());
        break;
      case ChangeMyPositionModeMessage::TYPE_STOP_FOLLOW:
        m_myPositionController->StopLocationFollow();
        break;
      case ChangeMyPositionModeMessage::TYPE_INVALIDATE:
        m_myPositionController->Invalidate();
        break;
      case ChangeMyPositionModeMessage::TYPE_CANCEL:
        m_myPositionController->TurnOff();
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
      ref_ptr<GpsInfoMessage> msg = message;
      m_myPositionController->OnLocationUpdate(msg->GetInfo(), msg->IsNavigable(),
                                               m_userEventStream.GetCurrentScreen());

      location::RouteMatchingInfo const & info = msg->GetRouteInfo();
      if (info.HasDistanceFromBegin())
        m_routeRenderer->UpdateDistanceFromBegin(info.GetDistanceFromBegin());

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

      if (m_selectionShape == nullptr)
        break;

      if (msg->IsDismiss())
      {
        m_selectionShape->Hide();
      }
      else
      {
        double offsetZ = 0.0;
        if (m_userEventStream.GetCurrentScreen().isPerspective())
        {
          dp::OverlayTree::TSelectResult selectResult;
          m_overlayTree->Select(msg->GetPosition(), selectResult);
          for (ref_ptr<dp::OverlayHandle> handle : selectResult)
            offsetZ = max(offsetZ, handle->GetPivotZ());
        }
        m_selectionShape->Show(msg->GetSelectedObject(), msg->GetPosition(), offsetZ, msg->IsAnim());
      }
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
      m2::PointD const finishPoint = routeData->m_sourcePolyline.Back();
      m_routeRenderer->SetRouteData(move(routeData), make_ref(m_gpuProgramManager));
      if (!m_routeRenderer->GetFinishPoint())
      {
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<CacheRouteSignMessage>(finishPoint, false /* isStart */,
                                                                        true /* isValid */),
                                  MessagePriority::High);
      }

      m_myPositionController->ActivateRouting();
#ifdef OMIM_OS_ANDROID
      if (m_pendingFollowRoute != nullptr)
      {
        FollowRoute(m_pendingFollowRoute->m_preferredZoomLevel, m_pendingFollowRoute->m_preferredZoomLevelIn3d,
                    m_pendingFollowRoute->m_rotationAngle, m_pendingFollowRoute->m_angleFOV);
        m_pendingFollowRoute.reset();
      }
#endif
      break;
    }

  case Message::FlushRouteSign:
    {
      ref_ptr<FlushRouteSignMessage> msg = message;
      drape_ptr<RouteSignData> routeSignData = msg->AcceptRouteSignData();
      m_routeRenderer->SetRouteSign(move(routeSignData), make_ref(m_gpuProgramManager));
      break;
    }

  case Message::RemoveRoute:
    {
      ref_ptr<RemoveRouteMessage> msg = message;
      m_routeRenderer->Clear();
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
#ifdef OMIM_OS_ANDROID
      // After night style switching on android and drape engine reinitialization FrontendRenderer
      // receive FollowRoute message before FlushRoute message, so we need to postpone its processing.
      if (!m_myPositionController->IsInRouting())
      {
        m_pendingFollowRoute.reset(
              new FollowRouteData(msg->GetPreferredZoomLevel(), msg->GetPreferredZoomLevelIn3d(),
                                  msg->GetRotationAngle(), msg->GetAngleFOV()));
        break;
      }
#else
      ASSERT(m_myPositionController->IsInRouting(), ());
#endif
      FollowRoute(msg->GetPreferredZoomLevel(), msg->GetPreferredZoomLevelIn3d(),
                  msg->GetRotationAngle(), msg->GetAngleFOV());
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

  case Message::UpdateMapStyle:
    {
      // Clear tile tree.
      m_tileTree->Invalidate();

      // Clear all graphics.
      for (RenderLayer & layer : m_layers)
      {
        layer.m_renderGroups.clear();
        layer.m_deferredRenderGroups.clear();
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

      // Invalidate route.
      if (m_routeRenderer->GetStartPoint())
      {
        m2::PointD const & position = m_routeRenderer->GetStartPoint()->m_position;
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<CacheRouteSignMessage>(position, true /* isStart */,
                                                                        true /* isValid */),
                                  MessagePriority::High);
      }
      if (m_routeRenderer->GetFinishPoint())
      {
        m2::PointD const & position = m_routeRenderer->GetFinishPoint()->m_position;
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<CacheRouteSignMessage>(position, false /* isStart */,
                                                                        true /* isValid */),
                                  MessagePriority::High);
      }

      auto const & routeData = m_routeRenderer->GetRouteData();
      if (routeData != nullptr)
      {
        auto recacheRouteMsg = make_unique_dp<AddRouteMessage>(routeData->m_sourcePolyline,
                                                               routeData->m_sourceTurns,
                                                               routeData->m_color);
        m_routeRenderer->Clear(true /* keepDistanceFromBegin */);
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, move(recacheRouteMsg),
                                  MessagePriority::Normal);
      }

      // Request new tiles.
      TTilesCollection tiles;
      ScreenBase screen = m_userEventStream.GetCurrentScreen();
      ResolveTileKeys(screen.ClipRect(), tiles);

      m_requestedTiles->Set(screen, m_isIsometry || screen.isPerspective(), move(tiles));
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<UpdateReadManagerMessage>(),
                                MessagePriority::UberHighSingleton);

      m_gpsTrackRenderer->Update();

      break;
    }

  case Message::EnablePerspective:
    {
      ref_ptr<EnablePerspectiveMessage> const msg = message;
      AddUserEvent(EnablePerspectiveEvent(msg->GetRotationAngle(), msg->GetAngleFOV(),
                                          false /* animated */, true /* immediately start */));
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
        if (m_enablePerspectiveInNavigation)
          AddUserEvent(EnablePerspectiveEvent(msg->GetRotationAngle(), msg->GetAngleFOV(),
                                              false /* animated */, true /* immediately start */));
        else
          AddUserEvent(DisablePerspectiveEvent());
      }
#endif

      if (m_enable3dBuildings != msg->Allow3dBuildings())
      {
        m_enable3dBuildings = msg->Allow3dBuildings();
        CheckIsometryMinScale(screen);
        InvalidateRect(screen.ClipRect());
      }

      if (m_enablePerspectiveInNavigation != msg->AllowPerspective())
      {
        m_enablePerspectiveInNavigation = msg->AllowPerspective();
        if (m_myPositionController->IsInRouting())
        {
          if (m_enablePerspectiveInNavigation && !screen.isPerspective() && !m_perspectiveDiscarded)
          {
            AddUserEvent(EnablePerspectiveEvent(msg->GetRotationAngle(), msg->GetAngleFOV(),
                                                true /* animated */, true /* immediately start */));
          }
          else if (!m_enablePerspectiveInNavigation && (screen.isPerspective() || m_perspectiveDiscarded))
          {
            DisablePerspective();
          }
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

  case Message::Invalidate:
    {
      // Do nothing here, new frame will be rendered because of this message processing.
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

void FrontendRenderer::FollowRoute(int preferredZoomLevel, int preferredZoomLevelIn3d,
                                   double rotationAngle, double angleFOV)
{
  if (m_enablePerspectiveInNavigation)
  {
    bool immediatelyStart = !m_myPositionController->IsRotationActive();
    AddUserEvent(EnablePerspectiveEvent(rotationAngle, angleFOV,
                                        true /* animated */, immediatelyStart));
  }

  m_myPositionController->NextMode(!m_enablePerspectiveInNavigation ? preferredZoomLevel
                                                                    : preferredZoomLevelIn3d);
  m_overlayTree->SetFollowingMode(true);

}

void FrontendRenderer::InvalidateRect(m2::RectD const & gRect)
{
  TTilesCollection tiles;
  ScreenBase screen = m_userEventStream.GetCurrentScreen();
  m2::RectD rect = gRect;
  if (rect.Intersect(screen.ClipRect()))
  {
    m_tileTree->Invalidate();
    ResolveTileKeys(rect, tiles);

    auto eraseFunction = [&tiles](drape_ptr<RenderGroup> && group)
    {
        return tiles.count(group->GetTileKey()) == 0;
    };

    for (RenderLayer & layer : m_layers)
    {
      RemoveGroups(eraseFunction, layer.m_renderGroups);
      RemoveGroups(eraseFunction, layer.m_deferredRenderGroups);
      layer.m_isDirty = true;
    }

    BaseBlockingMessage::Blocker blocker;
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<InvalidateReadManagerRectMessage>(blocker, tiles),
                              MessagePriority::High);
    blocker.Wait();

    m_requestedTiles->Set(screen, m_isIsometry || screen.isPerspective(), move(tiles));
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<UpdateReadManagerMessage>(),
                              MessagePriority::UberHighSingleton);
  }
}

void FrontendRenderer::OnResize(ScreenBase const & screen)
{
  m2::RectD const viewportRect = screen.isPerspective() ? screen.PixelRectIn3d() : screen.PixelRect();

  m_myPositionController->UpdatePixelPosition(screen);
  m_myPositionController->OnNewPixelRect();

  m_viewport.SetViewport(0, 0, viewportRect.SizeX(), viewportRect.SizeY());
  m_contextFactory->getDrawContext()->resize(viewportRect.SizeX(), viewportRect.SizeY());
  RefreshProjection(screen);
  RefreshPivotTransform(screen);

  m_framebuffer->SetSize(viewportRect.SizeX(), viewportRect.SizeY());
}

void FrontendRenderer::AddToRenderGroup(vector<drape_ptr<RenderGroup>> & groups,
                                        dp::GLState const & state,
                                        drape_ptr<dp::RenderBucket> && renderBucket,
                                        TileKey const & newTile)
{
  drape_ptr<RenderGroup> group = make_unique_dp<RenderGroup>(state, newTile);
  ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
  ref_ptr<dp::GpuProgram> program3d = m_gpuProgramManager->GetProgram(state.GetProgram3dIndex());

  group->SetRenderParams(program, program3d, make_ref(&m_generalUniforms));
  group->AddBucket(move(renderBucket));
  groups.push_back(move(group));
}

void FrontendRenderer::OnAddRenderGroup(TileKey const & tileKey, dp::GLState const & state,
                                        drape_ptr<dp::RenderBucket> && renderBucket)
{
  RenderLayer::RenderLayerID id = RenderLayer::GetLayerID(state);
  RenderLayer & layer = m_layers[id];
  AddToRenderGroup(layer.m_renderGroups, state, move(renderBucket), tileKey);
  layer.m_renderGroups.back()->Appear();
  layer.m_isDirty = true;
}

void FrontendRenderer::OnDeferRenderGroup(TileKey const & tileKey, dp::GLState const & state,
                                          drape_ptr<dp::RenderBucket> && renderBucket)
{
  RenderLayer::RenderLayerID id = RenderLayer::GetLayerID(state);
  AddToRenderGroup(m_layers[id].m_deferredRenderGroups, state, move(renderBucket), tileKey);
}

void FrontendRenderer::OnActivateTile(TileKey const & tileKey)
{
  for (RenderLayer & layer : m_layers)
  {
    MoveTileFunctor<ActivateTilePredicate> f(ActivateTilePredicate(tileKey), layer.m_renderGroups);
    layer.m_isDirty |= RemoveGroups(f, layer.m_deferredRenderGroups);
  }
}

void FrontendRenderer::OnRemoveTile(TileKey const & tileKey)
{
  auto removePredicate = [&tileKey](drape_ptr<RenderGroup> const & group)
  {
    return group->GetTileKey() == tileKey;
  };
  RemoveRenderGroups(removePredicate);
}

void FrontendRenderer::RemoveRenderGroups(TRenderGroupRemovePredicate const & predicate)
{
  ASSERT(predicate != nullptr, ());
  m_overlayTree->ForceUpdate();

  for (RenderLayer & layer : m_layers)
  {
    RemoveTilePredicate f(predicate);
    RemoveGroups(f, layer.m_renderGroups);
    RemoveGroups(predicate, layer.m_deferredRenderGroups);
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
  RemoveRenderGroups(removePredicate);

  return result;
}

void FrontendRenderer::OnCompassTapped()
{
  if (!m_myPositionController->StopCompassFollow())
    m_userEventStream.AddEvent(RotateEvent(0.0));
}

FeatureID FrontendRenderer::GetVisiblePOI(m2::PointD const & pixelPoint) const
{
  double halfSize = VisualParams::Instance().GetTouchRectRadius();
  m2::PointD sizePoint(halfSize, halfSize);
  m2::RectD selectRect(pixelPoint - sizePoint, pixelPoint + sizePoint);
  return GetVisiblePOI(selectRect);
}

FeatureID FrontendRenderer::GetVisiblePOI(m2::RectD const & pixelRect) const
{
  m2::PointD pt = pixelRect.Center();
  dp::OverlayTree::TSelectResult selectResult;
  m_overlayTree->Select(pixelRect, selectResult);

  double dist = numeric_limits<double>::max();
  FeatureID featureID;

  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  for (ref_ptr<dp::OverlayHandle> handle : selectResult)
  {
    double const curDist = pt.SquareLength(handle->GetPivot(screen, screen.isPerspective()));
    if (curDist < dist)
    {
      dist = curDist;
      featureID = handle->GetFeatureID();
    }
  }

  return featureID;
}

void FrontendRenderer::PrepareGpsTrackPoints(size_t pointsCount)
{
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                            make_unique_dp<CacheGpsTrackPointsMessage>(pointsCount),
                            MessagePriority::Normal);
}

void FrontendRenderer::BeginUpdateOverlayTree(ScreenBase const & modelView)
{
  if (m_overlayTree->Frame(modelView.isPerspective()))
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
    m_overlayTree->EndOverlayPlacing();
}

void FrontendRenderer::RenderScene(ScreenBase const & modelView)
{
#ifdef DRAW_INFO
  BeforeDrawFrame();
#endif

  bool const isPerspective = modelView.isPerspective();

  GLFunctions::glEnable(gl_const::GLDepthTest);
  m_viewport.Apply();
  RefreshBgColor();
  GLFunctions::glClear();

  Render2dLayer(modelView);

  bool hasSelectedPOI = false;
  if (m_selectionShape != nullptr)
  {
    GLFunctions::glDisable(gl_const::GLDepthTest);
    SelectionShape::ESelectedObject selectedObject = m_selectionShape->GetSelectedObject();
    if (selectedObject == SelectionShape::OBJECT_MY_POSITION)
    {
      ASSERT(m_myPositionController->IsModeHasPosition(), ());
      m_selectionShape->SetPosition(m_myPositionController->Position());
      m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
    }
    else if (selectedObject == SelectionShape::OBJECT_POI)
    {
      if (!isPerspective && m_layers[RenderLayer::Geometry3dID].m_renderGroups.empty())
        m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
      else
        hasSelectedPOI = true;
    }
  }

  m_myPositionController->Render(MyPositionController::RenderAccuracy,
                                 modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  if (m_framebuffer->IsSupported())
  {
    m_framebuffer->Enable();
    GLFunctions::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GLFunctions::glClear();
    Render3dLayer(modelView);
    m_framebuffer->Disable();
    GLFunctions::glDisable(gl_const::GLDepthTest);
    m_transparentLayer->Render(m_framebuffer->GetTextureId(), make_ref(m_gpuProgramManager));
  }
  else
  {
    GLFunctions::glClearDepth();
    Render3dLayer(modelView);
  }

  if (hasSelectedPOI)
  {
    GLFunctions::glDisable(gl_const::GLDepthTest);
    m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
  }

  GLFunctions::glEnable(gl_const::GLDepthTest);
  GLFunctions::glClearDepth();
  RenderOverlayLayer(modelView);

  m_gpsTrackRenderer->RenderTrack(modelView, GetCurrentZoomLevel(),
                                  make_ref(m_gpuProgramManager), m_generalUniforms);

  GLFunctions::glDisable(gl_const::GLDepthTest);
  if (m_selectionShape != nullptr && m_selectionShape->GetSelectedObject() == SelectionShape::OBJECT_USER_MARK)
    m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  m_routeRenderer->RenderRoute(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  for (drape_ptr<UserMarkRenderGroup> const & group : m_userMarkRenderGroups)
  {
    ASSERT(group.get() != nullptr, ());
    if (m_userMarkVisibility.find(group->GetTileKey()) != m_userMarkVisibility.end())
      RenderSingleGroup(modelView, make_ref(group));
  }

  m_routeRenderer->RenderRouteSigns(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  m_myPositionController->Render(MyPositionController::RenderMyPosition,
                                 modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  if (m_guiRenderer != nullptr)
  {
    if (isPerspective)
    {
      ScreenBase modelView2d = modelView;
      modelView2d.ResetPerspective();
      m_guiRenderer->Render(make_ref(m_gpuProgramManager), modelView2d);
    }
    else
    {
      m_guiRenderer->Render(make_ref(m_gpuProgramManager), modelView);
    }
  }

  GLFunctions::glEnable(gl_const::GLDepthTest);

#if defined(RENDER_DEBUG_RECTS) && defined(COLLECT_DISPLACEMENT_INFO)
  for (auto const & arrow : m_overlayTree->GetDisplacementInfo())
    dp::DebugRectRenderer::Instance().DrawArrow(modelView, arrow);
#endif

#ifdef DRAW_INFO
  AfterDrawFrame();
#endif

  MergeBuckets();
}

void FrontendRenderer::Render2dLayer(ScreenBase const & modelView)
{
  RenderLayer & layer2d = m_layers[RenderLayer::Geometry2dID];
  layer2d.Sort();

  for (drape_ptr<RenderGroup> const & group : layer2d.m_renderGroups)
    RenderSingleGroup(modelView, make_ref(group));
}

void FrontendRenderer::Render3dLayer(ScreenBase const & modelView)
{
  GLFunctions::glEnable(gl_const::GLDepthTest);
  RenderLayer & layer = m_layers[RenderLayer::Geometry3dID];
  layer.Sort();
  for (drape_ptr<RenderGroup> const & group : layer.m_renderGroups)
    RenderSingleGroup(modelView, make_ref(group));
}

void FrontendRenderer::RenderOverlayLayer(ScreenBase const & modelView)
{
  RenderLayer & overlay = m_layers[RenderLayer::OverlayID];
  overlay.Sort();
  BeginUpdateOverlayTree(modelView);
  for (drape_ptr<RenderGroup> & group : overlay.m_renderGroups)
    UpdateOverlayTree(modelView, group);
  EndUpdateOverlayTree();
  for (drape_ptr<RenderGroup> & group : overlay.m_renderGroups)
    RenderSingleGroup(modelView, make_ref(group));
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
        newGroups.push_back(move(layer.m_renderGroups[i]));
    }

    for (TGroupMap::value_type & node : forMerge)
      BatchMergeHelper::MergeBatches(node.second, newGroups, isPerspective);

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

void FrontendRenderer::RefreshModelView(ScreenBase const & screen)
{
  ScreenBase::MatrixT const & m = screen.GtoPMatrix();
  math::Matrix<float, 4, 4> mv;

  /// preparing ModelView matrix

  mv(0, 0) = m(0, 0); mv(0, 1) = m(1, 0); mv(0, 2) = 0; mv(0, 3) = m(2, 0);
  mv(1, 0) = m(0, 1); mv(1, 1) = m(1, 1); mv(1, 2) = 0; mv(1, 3) = m(2, 1);
  mv(2, 0) = 0;       mv(2, 1) = 0;       mv(2, 2) = 1; mv(2, 3) = 0;
  mv(3, 0) = m(0, 2); mv(3, 1) = m(1, 2); mv(3, 2) = 0; mv(3, 3) = m(2, 2);

  m_generalUniforms.SetMatrix4x4Value("modelView", mv.m_data);

  float const zScale = 2.0f / (screen.GetHeight() * screen.GetScale());
  m_generalUniforms.SetFloatValue("zScale", zScale);
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

int FrontendRenderer::GetCurrentZoomLevel() const
{
  return m_currentZoomLevel;
}

int FrontendRenderer::GetCurrentZoomLevelForData() const
{
  int const upperScale = scales::GetUpperScale();
  return (m_currentZoomLevel <= upperScale ? m_currentZoomLevel : upperScale);
}

void FrontendRenderer::DisablePerspective()
{
  m_perspectiveDiscarded = false;
  AddUserEvent(DisablePerspectiveEvent());
}

void FrontendRenderer::CheckIsometryMinScale(const ScreenBase &screen)
{
  bool const isScaleAllowableIn3d = UserEventStream::IsScaleAllowableIn3d(m_currentZoomLevel);
  bool const isIsometry = m_enable3dBuildings && isScaleAllowableIn3d;
  if (m_isIsometry != isIsometry)
  {
    m_isIsometry = isIsometry;
    RefreshPivotTransform(screen);
  }
}

void FrontendRenderer::CheckPerspectiveMinScale()
{
  if (!m_enablePerspectiveInNavigation || m_userEventStream.IsInPerspectiveAnimation())
    return;

  bool const switchTo2d = !UserEventStream::IsScaleAllowableIn3d(m_currentZoomLevel);
  if ((!switchTo2d && !m_perspectiveDiscarded) ||
      (switchTo2d && !m_userEventStream.GetCurrentScreen().isPerspective()))
    return;

  m_perspectiveDiscarded = switchTo2d;
  AddUserEvent(SwitchViewModeEvent(switchTo2d));
}

void FrontendRenderer::ResolveZoomLevel(ScreenBase const & screen)
{
  m_currentZoomLevel = GetDrawTileScale(screen);

  CheckIsometryMinScale(screen);
  CheckPerspectiveMinScale();
}

void FrontendRenderer::OnTap(m2::PointD const & pt, bool isLongTap)
{
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

  m_tapEventInfoFn(pt, isLongTap, isMyPosition, GetVisiblePOI(selectRect));
}

void FrontendRenderer::OnForceTap(m2::PointD const & pt)
{
  // Emulate long tap on force tap.
  OnTap(pt, true /* isLongTap */);
}

void FrontendRenderer::OnDoubleTap(m2::PointD const & pt)
{
  m_userEventStream.AddEvent(ScaleEvent(2.0 /* scale factor */, pt, true /* animated */));
}

void FrontendRenderer::OnTwoFingersTap()
{
  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  m_userEventStream.AddEvent(ScaleEvent(0.5 /* scale factor */, screen.PixelRect().Center(), true /* animated */));
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
}

void FrontendRenderer::OnAnimationStarted(ref_ptr<BaseModelViewAnimation> anim)
{
  m_myPositionController->AnimationStarted(anim);
}

void FrontendRenderer::ResolveTileKeys(ScreenBase const & screen, TTilesCollection & tiles)
{
  m2::RectD const & clipRect = screen.ClipRect();
  ResolveTileKeys(clipRect, tiles);
}

void FrontendRenderer::ResolveTileKeys(m2::RectD const & rect, TTilesCollection & tiles)
{
  // equal for x and y
  int const zoomLevel = GetCurrentZoomLevelForData();

  double const range = MercatorBounds::maxX - MercatorBounds::minX;
  double const rectSize = range / (1 << zoomLevel);

  int const minTileX = static_cast<int>(floor(rect.minX() / rectSize));
  int const maxTileX = static_cast<int>(ceil(rect.maxX() / rectSize));
  int const minTileY = static_cast<int>(floor(rect.minY() / rectSize));
  int const maxTileY = static_cast<int>(ceil(rect.maxY() / rectSize));

  // request new tiles
  m_tileTree->BeginRequesting(zoomLevel, rect);
  for (int tileY = minTileY; tileY < maxTileY; ++tileY)
  {
    for (int tileX = minTileX; tileX < maxTileX; ++tileX)
    {
      TileKey key(tileX, tileY, zoomLevel);
      if (rect.IsIntersect(key.GetGlobalRect()))
      {
        key.m_styleZoomLevel = GetCurrentZoomLevel();
        tiles.insert(key);
        m_tileTree->RequestTile(key);
      }
    }
  }
  m_tileTree->EndRequesting();
}

FrontendRenderer::Routine::Routine(FrontendRenderer & renderer) : m_renderer(renderer) {}

void FrontendRenderer::Routine::Do()
{
  gui::DrapeGui::Instance().ConnectOnCompassTappedHandler(bind(&FrontendRenderer::OnCompassTapped, &m_renderer));
  m_renderer.m_myPositionController->SetListener(ref_ptr<MyPositionController::Listener>(&m_renderer));
  m_renderer.m_userEventStream.SetListener(ref_ptr<UserEventStream::Listener>(&m_renderer));

  m_renderer.m_tileTree->SetHandlers(bind(&FrontendRenderer::OnAddRenderGroup, &m_renderer, _1, _2, _3),
                                     bind(&FrontendRenderer::OnDeferRenderGroup, &m_renderer, _1, _2, _3),
                                     bind(&FrontendRenderer::OnActivateTile, &m_renderer, _1),
                                     bind(&FrontendRenderer::OnRemoveTile, &m_renderer, _1));

  dp::OGLContext * context = m_renderer.m_contextFactory->getDrawContext();
  context->makeCurrent();
  m_renderer.m_framebuffer->SetDefaultContext(context);
  GLFunctions::Init();
  GLFunctions::AttachCache(this_thread::get_id());

  dp::SupportManager::Instance().Init();

  GLFunctions::glPixelStore(gl_const::GLUnpackAlignment, 1);
  GLFunctions::glEnable(gl_const::GLDepthTest);

  GLFunctions::glClearDepthValue(1.0);
  GLFunctions::glDepthFunc(gl_const::GLLessOrEqual);
  GLFunctions::glDepthMask(true);

  GLFunctions::glFrontFace(gl_const::GLClockwise);
  GLFunctions::glCullFace(gl_const::GLBack);
  GLFunctions::glEnable(gl_const::GLCullFace);

  m_renderer.m_gpuProgramManager->Init();

  dp::BlendingParams blendingParams;
  blendingParams.Apply();

#ifdef RENDER_DEBUG_RECTS
  dp::DebugRectRenderer::Instance().Init(make_ref(m_renderer.m_gpuProgramManager));
#endif

  double const kMaxInactiveSeconds = 2.0;

  my::Timer timer;
  my::Timer activityTimer;

  double frameTime = 0.0;
  bool modelViewChanged = true;
  bool viewportChanged = true;

  while (!IsCancelled())
  {
    ScreenBase modelView = m_renderer.ProcessEvents(modelViewChanged, viewportChanged);
    if (viewportChanged)
      m_renderer.OnResize(modelView);

    context->setDefaultFramebuffer();

    if (modelViewChanged || viewportChanged)
      m_renderer.PrepareScene(modelView);

    // Check for a frame is active.
    bool isActiveFrame = modelViewChanged || viewportChanged;

    isActiveFrame |= m_renderer.m_texMng->UpdateDynamicTextures();
    m_renderer.RenderScene(modelView);

    isActiveFrame |= InterpolationHolder::Instance().Advance(frameTime);

    if (modelViewChanged)
    {
      m_renderer.UpdateScene(modelView);
      m_renderer.EmitModelViewChanged(modelView);
    }

    isActiveFrame |= m_renderer.m_userEventStream.IsWaitingForActionCompletion();

    if (isActiveFrame)
      activityTimer.Reset();

    if (activityTimer.ElapsedSeconds() > kMaxInactiveSeconds)
    {
      // Process a message or wait for a message.
      m_renderer.ProcessSingleMessage();
      activityTimer.Reset();
    }
    else
    {
      double availableTime = VSyncInterval - timer.ElapsedSeconds();
      do
      {
        if (!m_renderer.ProcessSingleMessage(false /* waitForMessage */))
          break;

        activityTimer.Reset();
        availableTime = VSyncInterval - timer.ElapsedSeconds();
      }
      while (availableTime > 0);
    }

    context->present();
    frameTime = timer.ElapsedSeconds();
    timer.Reset();

    // Limit fps in following mode.
    double constexpr kFrameTime = 1.0 / 30.0;
    if (m_renderer.m_myPositionController->IsFollowingActive() && frameTime < kFrameTime)
    {
      uint32_t const ms = static_cast<uint32_t>((kFrameTime - frameTime) * 1000);
      this_thread::sleep_for(milliseconds(ms));
    }

    m_renderer.CheckRenderingEnabled();
  }

#ifdef RENDER_DEBUG_RECTS
  dp::DebugRectRenderer::Instance().Destroy();
#endif

  m_renderer.ReleaseResources();
}

void FrontendRenderer::ReleaseResources()
{
  m_tileTree.reset();
  for (RenderLayer & layer : m_layers)
  {
    layer.m_renderGroups.clear();
    layer.m_deferredRenderGroups.clear();
  }
  m_userMarkRenderGroups.clear();
  m_guiRenderer.reset();
  m_myPositionController.reset();
  m_selectionShape.release();
  m_routeRenderer.reset();
  m_framebuffer.reset();
  m_transparentLayer.reset();

  m_gpuProgramManager.reset();
  m_contextFactory->getDrawContext()->doneCurrent();
}

void FrontendRenderer::AddUserEvent(UserEvent const & event)
{
  m_userEventStream.AddEvent(event);
  if (IsInInfinityWaiting())
    CancelMessageWaiting();
}

void FrontendRenderer::PositionChanged(m2::PointD const & position)
{
  m_userPositionChangedFn(position);
}

void FrontendRenderer::ChangeModelView(m2::PointD const & center)
{
  AddUserEvent(SetCenterEvent(center, -1, true));
}

void FrontendRenderer::ChangeModelView(double azimuth)
{
  AddUserEvent(RotateEvent(azimuth));
}

void FrontendRenderer::ChangeModelView(m2::RectD const & rect)
{
  AddUserEvent(SetRectEvent(rect, true, scales::GetUpperComfortScale(), true));
}

void FrontendRenderer::ChangeModelView(m2::PointD const & userPos, double azimuth,
                                       m2::PointD const & pxZero, int preferredZoomLevel)
{
  AddUserEvent(FollowAndRotateEvent(userPos, pxZero, azimuth, preferredZoomLevel, true));
}

ScreenBase const & FrontendRenderer::ProcessEvents(bool & modelViewChanged, bool & viewportChanged)
{
  ScreenBase const & modelView = m_userEventStream.ProcessEvents(modelViewChanged, viewportChanged);
  gui::DrapeGui::Instance().SetInUserAction(m_userEventStream.IsInUserAction());

  return modelView;
}

void FrontendRenderer::PrepareScene(ScreenBase const & modelView)
{
  RefreshModelView(modelView);
  RefreshPivotTransform(modelView);

  m_myPositionController->UpdatePixelPosition(modelView);
}

void FrontendRenderer::UpdateScene(ScreenBase const & modelView)
{
  ResolveZoomLevel(modelView);
  TTilesCollection tiles;
  ResolveTileKeys(modelView, tiles);

  m_gpsTrackRenderer->Update();

  auto removePredicate = [this](drape_ptr<RenderGroup> const & group)
  {
    return group->IsOverlay() && group->GetTileKey().m_styleZoomLevel > GetCurrentZoomLevel();
  };
  RemoveRenderGroups(removePredicate);

  m_requestedTiles->Set(modelView, m_isIsometry || modelView.isPerspective(), move(tiles));
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                            make_unique_dp<UpdateReadManagerMessage>(),
                            MessagePriority::UberHighSingleton);
}

void FrontendRenderer::EmitModelViewChanged(ScreenBase const & modelView) const
{
  m_modelViewChangedFn(modelView);
}

FrontendRenderer::RenderLayer::RenderLayerID FrontendRenderer::RenderLayer::GetLayerID(dp::GLState const & state)
{
  if (state.GetDepthLayer() == dp::GLState::OverlayLayer)
    return OverlayID;

  if (state.GetProgram3dIndex() == gpu::AREA_3D_PROGRAM)
    return Geometry3dID;

  return Geometry2dID;
}

void FrontendRenderer::RenderLayer::Sort()
{
  if (!m_isDirty)
    return;

  RenderGroupComparator comparator;
  sort(m_renderGroups.begin(), m_renderGroups.end(), ref(comparator));
  m_isDirty = comparator.m_pendingOnDeleteFound;

  while (!m_renderGroups.empty() && m_renderGroups.back()->CanBeDeleted())
    m_renderGroups.pop_back();
}

} // namespace df
