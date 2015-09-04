#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

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

const double VSyncInterval = 0.030;
//#ifdef DEBUG
//const double VSyncInterval = 0.030;
//#else
//const double VSyncInterval = 0.014;
//#endif

} // namespace

FrontendRenderer::FrontendRenderer(Params const & params)
  : BaseRenderer(ThreadsCommutator::RenderThread, params)
  , m_gpuProgramManager(new dp::GpuProgramManager())
  , m_routeRenderer(new RouteRenderer())
  , m_overlayTree(new dp::OverlayTree())
  , m_viewport(params.m_viewport)
  , m_userEventStream(params.m_isCountryLoadedFn)
  , m_modelViewChangedFn(params.m_modelViewChangedFn)
  , m_tapEventInfoFn(params.m_tapEventFn)
  , m_userPositionChangedFn(params.m_positionChangedFn)
  , m_tileTree(new TileTree())
{
#ifdef DRAW_INFO
  m_tpf = 0,0;
  m_fps = 0.0;
#endif

  ASSERT(m_tapEventInfoFn, ());
  ASSERT(m_userPositionChangedFn, ());

  m_myPositionController.reset(new MyPositionController(params.m_initMyPositionMode));
  m_myPositionController->SetModeListener(params.m_myPositionModeCallback);

  StartThread();
}

FrontendRenderer::~FrontendRenderer()
{
  StopThread();
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
      program->Bind();
      bucket->GetBuffer()->Build(program);
      if (!IsUserMarkLayer(key))
        m_tileTree->ProcessTile(key, GetCurrentZoomLevel(), state, move(bucket));
      else
        m_userMarkRenderGroups.emplace_back(make_unique_dp<UserMarkRenderGroup>(state, key, move(bucket)));
      break;
    }

  case Message::FinishReading:
    {
      ref_ptr<FinishReadingMessage> msg = message;
      m_tileTree->FinishTiles(msg->GetTiles(), GetCurrentZoomLevel());
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

        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<InvalidateReadManagerRectMessage>(tiles),
                                  MessagePriority::Normal);

        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<UpdateReadManagerMessage>(screen, move(tiles)),
                                  MessagePriority::Normal);
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
        m_myPositionController->NextMode();
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
      msg->SetFeatureID(GetVisiblePOI(m_userEventStream.GetCurrentScreen().GtoP(msg->GetPoint())));
      break;
    }

  case Message::SelectObject:
    {
      ref_ptr<SelectObjectMessage> msg = message;
      ASSERT(m_selectionShape != nullptr, ());
      if (msg->IsDismiss())
        m_selectionShape->Hide();
      else
        m_selectionShape->Show(msg->GetSelectedObject(), msg->GetPosition(), msg->IsAnim());
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
      m_routeRenderer->SetRouteData(move(routeData), make_ref(m_gpuProgramManager));

      m_myPositionController->ActivateRouting();
      break;
    }

  case Message::RemoveRoute:
    {
      ref_ptr<RemoveRouteMessage> msg = message;
      m_routeRenderer->Clear();
      if (msg->NeedDeactivateFollowing())
        m_myPositionController->DeactivateRouting();
      break;
    }

  case Message::UpdateMapStyle:
    {
      m_tileTree->Invalidate();

      TTilesCollection tiles;
      ScreenBase screen = m_userEventStream.GetCurrentScreen();
      ResolveTileKeys(screen.ClipRect(), tiles);

      m_renderGroups.clear();
      m_deferredRenderGroups.clear();

      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<InvalidateReadManagerRectMessage>(tiles),
                                MessagePriority::Normal);

      BaseBlockingMessage::Blocker blocker;
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<InvalidateTexturesMessage>(blocker),
                                MessagePriority::Normal);
      blocker.Wait();

      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<UpdateReadManagerMessage>(screen, move(tiles)),
                                MessagePriority::Normal);

      RefreshBgColor();

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
  m_viewport.SetViewport(0, 0, screen.GetWidth(), screen.GetHeight());
  m_myPositionController->SetPixelRect(screen.PixelRect());
  m_contextFactory->getDrawContext()->resize(m_viewport.GetWidth(), m_viewport.GetHeight());
  RefreshProjection();
}

void FrontendRenderer::AddToRenderGroup(vector<drape_ptr<RenderGroup>> & groups,
                                        dp::GLState const & state,
                                        drape_ptr<dp::RenderBucket> && renderBucket,
                                        TileKey const & newTile)
{
  drape_ptr<RenderGroup> group = make_unique_dp<RenderGroup>(state, newTile);
  group->AddBucket(move(renderBucket));
  groups.push_back(move(group));
}

void FrontendRenderer::OnAddRenderGroup(TileKey const & tileKey, dp::GLState const & state,
                                        drape_ptr<dp::RenderBucket> && renderBucket)
{
  AddToRenderGroup(m_renderGroups, state, move(renderBucket), tileKey);
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
    }
    else
    {
      ++it;
    }
  }
}

void FrontendRenderer::OnRemoveTile(TileKey const & tileKey)
{
  m_overlayTree->ForceUpdate();
  for(auto const & group : m_renderGroups)
  {
    if (group->GetTileKey() == tileKey)
    {
      group->DeleteLater();
      group->Disappear();
    }
  }

  auto removePredicate = [&tileKey](drape_ptr<RenderGroup> const & group)
  {
    return group->GetTileKey() == tileKey;
  };
  m_deferredRenderGroups.erase(remove_if(m_deferredRenderGroups.begin(),
                                         m_deferredRenderGroups.end(),
                                         removePredicate),
                               m_deferredRenderGroups.end());
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
    double const  curDist = pt.SquareLength(handle->GetPivot(screen));
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
  m_overlayTree->Frame();

  if (m_overlayTree->IsNeedUpdate())
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

  RenderGroupComparator comparator;
  sort(m_renderGroups.begin(), m_renderGroups.end(), bind(&RenderGroupComparator::operator (), &comparator, _1, _2));

  BeginUpdateOverlayTree(modelView);
  size_t eraseCount = 0;
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
  for (; currentRenderGroup < m_renderGroups.size(); ++currentRenderGroup)
  {
    drape_ptr<RenderGroup> const & group = m_renderGroups[currentRenderGroup];

    dp::GLState const & state = group->GetState();
    dp::GLState::DepthLayer layer = state.GetDepthLayer();
    if (prevLayer != layer && layer == dp::GLState::OverlayLayer)
      break;

    prevLayer = layer;
    RenderSingleGroup(modelView, make_ref(group));
  }

  GLFunctions::glClearDepth();
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
      m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
  }

  m_myPositionController->Render(MyPositionController::RenderAccuracy,
                                 modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  for (; currentRenderGroup < m_renderGroups.size(); ++currentRenderGroup)
  {
    drape_ptr<RenderGroup> const & group = m_renderGroups[currentRenderGroup];
    RenderSingleGroup(modelView, make_ref(group));
  }

  GLFunctions::glClearDepth();
  if (m_selectionShape != nullptr && m_selectionShape->GetSelectedObject() == SelectionShape::OBJECT_USER_MARK)
    m_selectionShape->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  for (drape_ptr<UserMarkRenderGroup> const & group : m_userMarkRenderGroups)
  {
    ASSERT(group.get() != nullptr, ());
    group->UpdateAnimation();
    if (m_userMarkVisibility.find(group->GetTileKey()) != m_userMarkVisibility.end())
      RenderSingleGroup(modelView, make_ref(group));
  }

  GLFunctions::glDisable(gl_const::GLDepthTest);

  m_routeRenderer->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
  m_myPositionController->Render(MyPositionController::RenderMyPosition,
                                 modelView, make_ref(m_gpuProgramManager), m_generalUniforms);

  if (m_guiRenderer != nullptr)
    m_guiRenderer->Render(make_ref(m_gpuProgramManager), modelView);

  GLFunctions::glEnable(gl_const::GLDepthTest);

#ifdef DRAW_INFO
  AfterDrawFrame();
#endif
}

void FrontendRenderer::RenderSingleGroup(ScreenBase const & modelView, ref_ptr<BaseRenderGroup> group)
{
  group->UpdateAnimation();
  dp::GLState const & state = group->GetState();

  ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
  program->Bind();
  ApplyUniforms(m_generalUniforms, program);
  ApplyUniforms(group->GetUniforms(), program);
  ApplyState(state, program);

  group->Render(modelView);
}

void FrontendRenderer::RefreshProjection()
{
  array<float, 16> m;

  dp::MakeProjection(m, 0.0f, m_viewport.GetWidth(), m_viewport.GetHeight(), 0.0f);
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
}

void FrontendRenderer::RefreshBgColor()
{
  uint32_t color = drule::rules().GetBgColor(df::GetDrawTileScale(m_userEventStream.GetCurrentScreen()));
  dp::Color c = dp::Extract(color, 255 - (color >> 24));
  GLFunctions::glClearColor(c.GetRedF(), c.GetGreenF(), c.GetBlueF(), 1.0f);
}

int FrontendRenderer::GetCurrentZoomLevel() const
{
  return m_currentZoomLevel;
}

void FrontendRenderer::ResolveZoomLevel(ScreenBase const & screen)
{
  m_currentZoomLevel = GetDrawTileScale(screen);
}

void FrontendRenderer::OnTap(m2::PointD const & pt, bool isLongTap)
{
  double halfSize = VisualParams::Instance().GetTouchRectRadius();
  m2::PointD sizePoint(halfSize, halfSize);
  m2::RectD selectRect(pt - sizePoint, pt + sizePoint);

  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  bool isMyPosition = false;
  if (m_myPositionController->IsModeHasPosition())
    isMyPosition = selectRect.IsPointInside(screen.GtoP(m_myPositionController->Position()));

  m_tapEventInfoFn(pt, isLongTap, isMyPosition, GetVisiblePOI(selectRect));
}

void FrontendRenderer::OnDoubleTap(m2::PointD const & pt)
{
  m_userEventStream.AddEvent(ScaleEvent(2.0 /*scale factor*/, pt, true /*animated*/));
}

bool FrontendRenderer::OnSingleTouchFiltrate(m2::PointD const & pt, TouchEvent::ETouchType type)
{
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
  int const tileScale = GetCurrentZoomLevel();
  double const range = MercatorBounds::maxX - MercatorBounds::minX;
  double const rectSize = range / (1 << tileScale);

  int const minTileX = static_cast<int>(floor(rect.minX() / rectSize));
  int const maxTileX = static_cast<int>(ceil(rect.maxX() / rectSize));
  int const minTileY = static_cast<int>(floor(rect.minY() / rectSize));
  int const maxTileY = static_cast<int>(ceil(rect.maxY() / rectSize));

  // request new tiles
  m_tileTree->BeginRequesting(tileScale, rect);
  for (int tileY = minTileY; tileY < maxTileY; ++tileY)
  {
    for (int tileX = minTileX; tileX < maxTileX; ++tileX)
    {
      TileKey key(tileX, tileY, tileScale);
      if (rect.IsIntersect(key.GetGlobalRect()))
      {
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
  GLFunctions::Init();
  GLFunctions::AttachCache(this_thread::get_id());

  GLFunctions::glPixelStore(gl_const::GLUnpackAlignment, 1);
  GLFunctions::glEnable(gl_const::GLDepthTest);

  m_renderer.RefreshBgColor();

  GLFunctions::glClearDepthValue(1.0);
  GLFunctions::glDepthFunc(gl_const::GLLessOrEqual);
  GLFunctions::glDepthMask(true);

  GLFunctions::glFrontFace(gl_const::GLClockwise);
  GLFunctions::glCullFace(gl_const::GLBack);
  GLFunctions::glEnable(gl_const::GLCullFace);

  dp::BlendingParams blendingParams;
  blendingParams.Apply();

  my::HighResTimer timer;
  //double processingTime = InitAvarageTimePerMessage; // By init we think that one message processed by 1ms

  timer.Reset();
  double frameTime = 0.0;
  int inactiveFrameCount = 0;
  bool viewChanged = true;
  ScreenBase modelView = m_renderer.UpdateScene(viewChanged);
  while (!IsCancelled())
  {
    context->setDefaultFramebuffer();
    bool const hasAsyncRoutines = m_renderer.m_texMng->UpdateDynamicTextures();
    m_renderer.RenderScene(modelView);
    bool const animActive = InterpolationHolder::Instance().Advance(frameTime);
    modelView = m_renderer.UpdateScene(viewChanged);

    if (!viewChanged && m_renderer.IsQueueEmpty() && !animActive && !hasAsyncRoutines)
      ++inactiveFrameCount;
    else
      inactiveFrameCount = 0;

    if (inactiveFrameCount > 60)
    {
      // process a message or wait for a message
      m_renderer.ProcessSingleMessage();
      inactiveFrameCount = 0;
    }
    else
    {
      double availableTime = VSyncInterval - (timer.ElapsedSeconds() /*+ avarageMessageTime*/);

      if (availableTime < 0.0)
        availableTime = 0.01;

      while (availableTime > 0)
      {
        m_renderer.ProcessSingleMessage(availableTime * 1000.0);
        availableTime = VSyncInterval - (timer.ElapsedSeconds() /*+ avarageMessageTime*/);
        //messageCount++;
      }

      //processingTime = (timer.ElapsedSeconds() - processingTime) / messageCount;
    }

    context->present();
    frameTime = timer.ElapsedSeconds();
    timer.Reset();

    m_renderer.CheckRenderingEnabled();
  }

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
                                       m2::PointD const & pxZero, bool animate)
{
  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  m2::RectD const & pixelRect = screen.PixelRect();

  m2::PointD formingVector = pixelRect.Center() - pxZero;
  formingVector.x /= pixelRect.SizeX();
  formingVector.y /= pixelRect.SizeY();

  m2::AnyRectD targetRect = m_userEventStream.GetTargetRect();
  formingVector.x *= targetRect.GetLocalRect().SizeX();
  formingVector.y *= targetRect.GetLocalRect().SizeY();

  m2::PointD viewVector = userPos.Move(1.0, -azimuth + math::pi2) - userPos;
  viewVector.Normalize();

  AddUserEvent(SetAnyRectEvent(m2::AnyRectD(userPos + (viewVector * formingVector.Length()), -azimuth,
                                            targetRect.GetLocalRect()), animate));
}

ScreenBase const & FrontendRenderer::UpdateScene(bool & modelViewChanged)
{
  bool viewportChanged;
  ScreenBase const & modelView = m_userEventStream.ProcessEvents(modelViewChanged, viewportChanged);
  gui::DrapeGui::Instance().SetInUserAction(m_userEventStream.IsInUserAction());
  if (viewportChanged)
    OnResize(modelView);

  if (modelViewChanged)
  {
    ResolveZoomLevel(modelView);
    TTilesCollection tiles;
    ResolveTileKeys(modelView, tiles);

    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<UpdateReadManagerMessage>(modelView, move(tiles)),
                              MessagePriority::High);

    RefreshModelView(modelView);
    RefreshBgColor();
    EmitModelViewChanged(modelView);
  }

  return modelView;
}

void FrontendRenderer::EmitModelViewChanged(ScreenBase const & modelView) const
{
  m_modelViewChangedFn(modelView);
}

} // namespace df
