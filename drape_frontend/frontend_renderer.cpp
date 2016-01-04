#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/framebuffer.hpp"
#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/transparent_layer.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/debug_rect_renderer.hpp"
#include "drape/shader_def.hpp"
#include "drape/support_manager.hpp"

#include "drape/utils/glyph_usage_tracker.hpp"
#include "drape/utils/gpu_mem_tracker.hpp"
#include "drape/utils/projection.hpp"

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

} // namespace

FrontendRenderer::FrontendRenderer(Params const & params)
  : BaseRenderer(ThreadsCommutator::RenderThread, params)
  , m_gpuProgramManager(new dp::GpuProgramManager())
  , m_routeRenderer(new RouteRenderer())
  , m_framebuffer(new Framebuffer())
  , m_transparentLayer(new TransparentLayer())
  , m_overlayTree(new dp::OverlayTree())
  , m_enablePerspectiveInNavigation(false)
  , m_enable3dBuildings(false)
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
      TTilesCollection tiles;
      ScreenBase screen = m_userEventStream.GetCurrentScreen();
      m2::RectD rect = m->GetRect();
      if (rect.Intersect(screen.ClipRect()))
      {
        m_tileTree->Invalidate();
        ResolveTileKeys(rect, tiles);

        auto eraseFunction = [&tiles](vector<drape_ptr<RenderGroup>> & groups)
        {
          vector<drape_ptr<RenderGroup> > newGroups;
          for (drape_ptr<RenderGroup> & group : groups)
          {
            if (tiles.find(group->GetTileKey()) == tiles.end())
              newGroups.push_back(move(group));
          }

          swap(groups, newGroups);
        };

        eraseFunction(m_renderGroups);
        eraseFunction(m_deferredRenderGroups);

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
      m2::PointD const startPoint = routeData->m_sourcePolyline.Front();
      m2::PointD const finishPoint = routeData->m_sourcePolyline.Back();
      m_routeRenderer->SetRouteData(move(routeData), make_ref(m_gpuProgramManager));
      if (!m_routeRenderer->GetStartPoint())
      {
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<CacheRouteSignMessage>(startPoint, true /* isStart */,
                                                                        true /* isValid */),
                                  MessagePriority::High);
      }
      if (!m_routeRenderer->GetFinishPoint())
      {
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<CacheRouteSignMessage>(finishPoint, false /* isStart */,
                                                                        true /* isValid */),
                                  MessagePriority::High);
      }

      m_myPositionController->ActivateRouting();
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
      m_myPositionController->NextMode(!m_enablePerspectiveInNavigation ? msg->GetPreferredZoomLevel()
                                                               : msg->GetPreferredZoomLevelIn3d());
      m_overlayTree->SetFollowingMode(true);
      if (m_enablePerspectiveInNavigation)
      {
        bool immediatelyStart = !m_myPositionController->IsRotationActive();
        AddUserEvent(EnablePerspectiveEvent(msg->GetRotationAngle(), msg->GetAngleFOV(),
                                            true /* animated */, immediatelyStart));
      }
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

      // Get new tiles.
      TTilesCollection tiles;
      ScreenBase screen = m_userEventStream.GetCurrentScreen();
      ResolveTileKeys(screen.ClipRect(), tiles);

      // Clear all graphics.
      m_renderGroups.clear();
      m_deferredRenderGroups.clear();

      // Invalidate read manager.
      {
        BaseBlockingMessage::Blocker blocker;
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<InvalidateReadManagerRectMessage>(blocker, tiles),
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
      m_requestedTiles->Set(screen, m_isIsometry || screen.isPerspective(), move(tiles));
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<UpdateReadManagerMessage>(),
                                MessagePriority::UberHighSingleton);

      RefreshBgColor();

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
      bool const isPerspective = m_userEventStream.GetCurrentScreen().isPerspective();
#ifdef OMIM_OS_DESKTOP
      if (m_enablePerspectiveInNavigation == msg->AllowPerspective() &&
          m_enablePerspectiveInNavigation != isPerspective)
      {
        if (m_enablePerspectiveInNavigation)
          AddUserEvent(EnablePerspectiveEvent(msg->GetRotationAngle(), msg->GetAngleFOV(),
                                              false /* animated */, true /* immediately start */));
        else
          AddUserEvent(DisablePerspectiveEvent());
      }
#endif
      m_enablePerspectiveInNavigation = msg->AllowPerspective();
      m_enable3dBuildings = msg->Allow3dBuildings();

      if (m_myPositionController->IsInRouting())
      {
        if (m_enablePerspectiveInNavigation && !isPerspective && !m_perspectiveDiscarded)
        {
          AddUserEvent(EnablePerspectiveEvent(msg->GetRotationAngle(), msg->GetAngleFOV(),
                                              true /* animated */, true /* immediately start */));
        }
        else if (!m_enablePerspectiveInNavigation && (isPerspective || m_perspectiveDiscarded))
        {
          DisablePerspective();
        }
      }
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

void FrontendRenderer::OnResize(ScreenBase const & screen)
{
  m2::RectD const viewportRect = screen.isPerspective() ? screen.PixelRectIn3d() : screen.PixelRect();

  m_myPositionController->UpdatePixelPosition(screen);
  m_myPositionController->SetPixelRect(viewportRect);

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
  AddToRenderGroup(m_renderGroups, state, move(renderBucket), tileKey);
  m_renderGroups.back()->Appear();
}

void FrontendRenderer::OnDeferRenderGroup(TileKey const & tileKey, dp::GLState const & state,
                                          drape_ptr<dp::RenderBucket> && renderBucket)
{
  AddToRenderGroup(m_deferredRenderGroups, state, move(renderBucket), tileKey);
}

void FrontendRenderer::OnActivateTile(TileKey const & tileKey)
{
  for(auto it = m_deferredRenderGroups.begin(); it != m_deferredRenderGroups.end();)
  {
    if ((*it)->GetTileKey() == tileKey)
    {
      m_renderGroups.push_back(move(*it));
      it = m_deferredRenderGroups.erase(it);
      m_renderGroups.back()->Appear();
    }
    else
    {
      ++it;
    }
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

  for(auto const & group : m_renderGroups)
  {
    if (predicate(group))
    {
      group->DeleteLater();
      group->Disappear();
    }
  }

  m_deferredRenderGroups.erase(remove_if(m_deferredRenderGroups.begin(),
                                         m_deferredRenderGroups.end(),
                                         predicate),
                               m_deferredRenderGroups.end());
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
  m_myPositionController->StopCompassFollow();
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

  RenderGroupComparator comparator;
  sort(m_renderGroups.begin(), m_renderGroups.end(), bind(&RenderGroupComparator::operator (), &comparator, _1, _2));

  BeginUpdateOverlayTree(modelView);
  size_t eraseCount = 0;
  GLFunctions::glEnable(gl_const::GLDepthTest);
  for (size_t i = 0; i < m_renderGroups.size(); ++i)
  {
    drape_ptr<RenderGroup> & group = m_renderGroups[i];
    if (group->IsEmpty())
      continue;

    if (group->IsPendingOnDelete())
    {
      group.reset();
      ++eraseCount;
      continue;
    }

    switch (group->GetState().GetDepthLayer())
    {
    case dp::GLState::OverlayLayer:
      UpdateOverlayTree(modelView, group);
      break;
    case dp::GLState::DynamicGeometry:
      group->Update(modelView);
      break;
    default:
      break;
    }
  }
  EndUpdateOverlayTree();
  m_renderGroups.resize(m_renderGroups.size() - eraseCount);

  m_viewport.Apply();
  GLFunctions::glClear();

  dp::GLState::DepthLayer prevLayer = dp::GLState::GeometryLayer;
  size_t currentRenderGroup = 0;
  bool has3dAreas = false;
  size_t area3dRenderGroupStart = 0;
  size_t area3dRenderGroupEnd = 0;
  for (; currentRenderGroup < m_renderGroups.size(); ++currentRenderGroup)
  {
    drape_ptr<RenderGroup> const & group = m_renderGroups[currentRenderGroup];

    dp::GLState const & state = group->GetState();

    if ((isPerspective || m_isIsometry) && state.GetProgram3dIndex() == gpu::AREA_3D_PROGRAM)
    {
      if (!has3dAreas)
      {
        area3dRenderGroupStart = currentRenderGroup;
        has3dAreas = true;
      }
      area3dRenderGroupEnd = currentRenderGroup;
      continue;
    }

    dp::GLState::DepthLayer layer = state.GetDepthLayer();
    if (prevLayer != layer && layer == dp::GLState::OverlayLayer)
      break;

    prevLayer = layer;
    RenderSingleGroup(modelView, make_ref(group));
  }

  //GLFunctions::glClearDepth();
  GLFunctions::glDisable(gl_const::GLDepthTest);
  bool hasSelectedPOI = false;
  if (m_selectionShape != nullptr)
  {
    SelectionShape::ESelectedObject selectedObject = m_selectionShape->GetSelectedObject();
    if (selectedObject == SelectionShape::OBJECT_MY_POSITION)
    {
      ASSERT(m_myPositionController->IsModeHasPosition(), ());
      m_selectionShape->SetPosition(m_myPositionController->Position());
      m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
    }
    else if (selectedObject == SelectionShape::OBJECT_POI)
    {
      hasSelectedPOI = true;
      if (!isPerspective)
        m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
    }
  }

  m_myPositionController->Render(MyPositionController::RenderAccuracy,
                                 modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  if (has3dAreas)
  {
    m_framebuffer->Enable();
    GLFunctions::glClear();
    GLFunctions::glClearDepth();

    GLFunctions::glEnable(gl_const::GLDepthTest);
    for (size_t index = area3dRenderGroupStart; index <= area3dRenderGroupEnd; ++index)
    {
      drape_ptr<RenderGroup> const & group = m_renderGroups[index];
      if (group->GetState().GetProgram3dIndex() == gpu::AREA_3D_PROGRAM)
        RenderSingleGroup(modelView, make_ref(group));
    }
    m_framebuffer->Disable();

    GLFunctions::glDisable(gl_const::GLDepthTest);
    m_transparentLayer->Render(m_framebuffer->GetTextureId(), make_ref(m_gpuProgramManager));
  }

  if (isPerspective && hasSelectedPOI)
  {
    GLFunctions::glDisable(gl_const::GLDepthTest);
    m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
  }

  GLFunctions::glEnable(gl_const::GLDepthTest);
  GLFunctions::glClearDepth();
  for (; currentRenderGroup < m_renderGroups.size(); ++currentRenderGroup)
  {
    drape_ptr<RenderGroup> const & group = m_renderGroups[currentRenderGroup];
    RenderSingleGroup(modelView, make_ref(group));
  }

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
  GLFunctions::glClearColor(c.GetRedF(), c.GetGreenF(), c.GetBlueF(), 0.0f);
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

void FrontendRenderer::CheckMinAllowableIn3dScale()
{
  bool const isScaleAllowableIn3d = UserEventStream::IsScaleAllowableIn3d(m_currentZoomLevel);
  m_isIsometry = m_enable3dBuildings && isScaleAllowableIn3d;

  if (!m_enablePerspectiveInNavigation || m_userEventStream.IsInPerspectiveAnimation())
    return;

  bool const switchTo2d = !isScaleAllowableIn3d;
  if ((!switchTo2d && !m_perspectiveDiscarded) ||
      (switchTo2d && !m_userEventStream.GetCurrentScreen().isPerspective()))
    return;

  m_perspectiveDiscarded = switchTo2d;
  AddUserEvent(SwitchViewModeEvent(switchTo2d));
}

void FrontendRenderer::ResolveZoomLevel(ScreenBase const & screen)
{
  m_currentZoomLevel = GetDrawTileScale(screen);

  CheckMinAllowableIn3dScale();
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

  m_renderer.RefreshBgColor();

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
  m_renderGroups.clear();
  m_deferredRenderGroups.clear();
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
  RefreshBgColor();
  RefreshPivotTransform(modelView);

  m_myPositionController->UpdatePixelPosition(modelView);
}

void FrontendRenderer::UpdateScene(ScreenBase const & modelView)
{
  ResolveZoomLevel(modelView);
  TTilesCollection tiles;
  ResolveTileKeys(modelView, tiles);

  m_overlayTree->ForceUpdate();
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

} // namespace df
