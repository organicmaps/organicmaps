#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape_gui/drape_gui.hpp"

#include "drape/utils/glyph_usage_tracker.hpp"
#include "drape/utils/gpu_mem_tracker.hpp"
#include "drape/utils/projection.hpp"

#include "indexer/scales.hpp"

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

#ifdef DEBUG
const double VSyncInterval = 0.030;
//const double InitAvarageTimePerMessage = 0.003;
#else
const double VSyncInterval = 0.014;
//const double InitAvarageTimePerMessage = 0.001;
#endif

} // namespace

FrontendRenderer::FrontendRenderer(Params const & params)
  : BaseRenderer(ThreadsCommutator::RenderThread, params)
  , m_gpuProgramManager(new dp::GpuProgramManager())
  , m_viewport(params.m_viewport)
  , m_userEventStream(params.m_isCountryLoadedFn)
  , m_modelViewChangedFn(params.m_modelViewChangedFn)
  , m_tileTree(new TileTree())
{
#ifdef DRAW_INFO
  m_tpf = 0,0;
  m_fps = 0.0;
#endif

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
      // TODO(@kuznetsov): implement invalidation

      //ref_ptr<InvalidateRectMessage> m = message;
      //TTilesCollection keyStorage;
      //Message * msgToBackend = new InvalidateReadManagerRectMessage(keyStorage);
      //m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
      //                          dp::MovePointer(msgToBackend),
      //                          MessagePriority::Normal);
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

  case Message::StopRendering:
    {
      ProcessStopRenderingMessage();
      break;
    }

  case Message::MyPositionShape:
    m_myPositionController->SetRenderShape(ref_ptr<MyPositionShapeMessage>(message)->AcceptShape());
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
      m_myPositionController->OnCompassUpdate(msg->GetInfo());
      break;
    }

  case Message::GpsInfo:
    {
      ref_ptr<GpsInfoMessage> msg = message;
      m_myPositionController->OnLocationUpdate(msg->GetInfo(), msg->IsNavigable());
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

  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                            make_unique_dp<ResizeMessage>(m_viewport),
                            MessagePriority::High);
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

void FrontendRenderer::RenderScene(ScreenBase const & modelView)
{
#ifdef DRAW_INFO
  BeforeDrawFrame();
#endif

  RenderGroupComparator comparator;
  sort(m_renderGroups.begin(), m_renderGroups.end(), bind(&RenderGroupComparator::operator (), &comparator, _1, _2));

  dp::OverlayTree overlayTree;
  overlayTree.StartOverlayPlacing(modelView);
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
      group->CollectOverlay(make_ref(&overlayTree));
      break;
    case dp::GLState::DynamicGeometry:
      group->Update(modelView);
      break;
    default:
      break;
    }
  }
  overlayTree.EndOverlayPlacing();
  m_renderGroups.resize(m_renderGroups.size() - eraseCount);

  m_viewport.Apply();
  GLFunctions::glClear();

  dp::GLState::DepthLayer prevLayer = dp::GLState::GeometryLayer;
  for (drape_ptr<RenderGroup> const & group : m_renderGroups)
  {
    group->UpdateAnimation();
    dp::GLState const & state = group->GetState();
    dp::GLState::DepthLayer layer = state.GetDepthLayer();
    if (prevLayer != layer && layer == dp::GLState::OverlayLayer)
    {
      GLFunctions::glClearDepth();
      m_myPositionController->Render(modelView, make_ref(m_gpuProgramManager), m_generalUniforms);
    }

    prevLayer = layer;
    ASSERT_LESS_OR_EQUAL(prevLayer, layer, ());

    ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
    program->Bind();
    ApplyUniforms(m_generalUniforms, program);
    ApplyUniforms(group->GetUniforms(), program);
    ApplyState(state, program);

    group->Render(modelView);
  }

  GLFunctions::glClearDepth();

  for (drape_ptr<UserMarkRenderGroup> const & group : m_userMarkRenderGroups)
  {
    ASSERT(group.get() != nullptr, ());
    group->UpdateAnimation();
    if (m_userMarkVisibility.find(group->GetTileKey()) != m_userMarkVisibility.end())
    {
      dp::GLState const & state = group->GetState();
      ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
      program->Bind();
      ApplyUniforms(m_generalUniforms, program);
      ApplyUniforms(group->GetUniforms(), program);
      ApplyState(state, program);
      group->Render(modelView);
    }
  }

  GLFunctions::glClearDepth();
  if (m_guiRenderer != nullptr)
    m_guiRenderer->Render(make_ref(m_gpuProgramManager), modelView);

#ifdef DRAW_INFO
  AfterDrawFrame();
#endif
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

int FrontendRenderer::GetCurrentZoomLevel() const
{
  return m_currentZoomLevel;
}

void FrontendRenderer::ResolveZoomLevel(const ScreenBase & screen)
{
  //TODO(@kuznetsov): revise it
  int const upperScale = scales::GetUpperScale();
  int const zoomLevel = GetDrawTileScale(screen);
  m_currentZoomLevel = (zoomLevel <= upperScale ? zoomLevel : upperScale);
}

void FrontendRenderer::TapDetected(m2::PointD const & pt, bool isLongTap)
{
  LOG(LINFO, ("Tap detected. Is long = ", isLongTap));
}

bool FrontendRenderer::SingleTouchFiltration(m2::PointD const & pt, TouchEvent::ETouchType type)
{
  switch(type)
  {
  case TouchEvent::ETouchType::TOUCH_DOWN:
    return m_guiRenderer->OnTouchDown(pt);

  case TouchEvent::ETouchType::TOUCH_UP:
    m_guiRenderer->OnTouchUp(pt);
    return false;

  case TouchEvent::ETouchType::TOUCH_CANCEL:
    m_guiRenderer->OnTouchCancel(pt);
    return false;

  case TouchEvent::ETouchType::TOUCH_MOVE:
    return false;
  }

  return false;
}

void FrontendRenderer::ResolveTileKeys(ScreenBase const & screen, TTilesCollection & tiles)
{
  // equal for x and y
  int const tileScale = GetCurrentZoomLevel();
  double const range = MercatorBounds::maxX - MercatorBounds::minX;
  double const rectSize = range / (1 << tileScale);

  m2::RectD const & clipRect = screen.ClipRect();

  int const minTileX = static_cast<int>(floor(clipRect.minX() / rectSize));
  int const maxTileX = static_cast<int>(ceil(clipRect.maxX() / rectSize));
  int const minTileY = static_cast<int>(floor(clipRect.minY() / rectSize));
  int const maxTileY = static_cast<int>(ceil(clipRect.maxY() / rectSize));

  // request new tiles
  m_tileTree->BeginRequesting(tileScale, clipRect);
  for (int tileY = minTileY; tileY < maxTileY; ++tileY)
  {
    for (int tileX = minTileX; tileX < maxTileX; ++tileX)
    {
      TileKey key(tileX, tileY, tileScale);
      if (clipRect.IsIntersect(key.GetGlobalRect()))
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

  m_renderer.m_tileTree->SetHandlers(bind(&FrontendRenderer::OnAddRenderGroup, &m_renderer, _1, _2, _3),
                                     bind(&FrontendRenderer::OnDeferRenderGroup, &m_renderer, _1, _2, _3),
                                     bind(&FrontendRenderer::OnActivateTile, &m_renderer, _1),
                                     bind(&FrontendRenderer::OnRemoveTile, &m_renderer, _1));

  m_renderer.m_userEventStream.SetTapListener(bind(&FrontendRenderer::TapDetected, &m_renderer, _1, _2),
                                              bind(&FrontendRenderer::SingleTouchFiltration, &m_renderer, _1, _2));

  dp::OGLContext * context = m_renderer.m_contextFactory->getDrawContext();
  context->makeCurrent();
  GLFunctions::Init();

  GLFunctions::glPixelStore(gl_const::GLUnpackAlignment, 1);
  GLFunctions::glEnable(gl_const::GLDepthTest);

  GLFunctions::glClearColor(0.93f, 0.93f, 0.86f, 1.f);
  GLFunctions::glClearDepthValue(1.0);
  GLFunctions::glDepthFunc(gl_const::GLLessOrEqual);
  GLFunctions::glDepthMask(true);

  GLFunctions::glFrontFace(gl_const::GLClockwise);
  GLFunctions::glCullFace(gl_const::GLBack);
  GLFunctions::glEnable(gl_const::GLCullFace);

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
    m_renderer.m_texMng->UpdateDynamicTextures();
    m_renderer.RenderScene(modelView);
    bool const animActive = InterpolationHolder::Instance().Advance(frameTime);
    modelView = m_renderer.UpdateScene(viewChanged);

    if (!viewChanged && m_renderer.IsQueueEmpty() && !animActive)
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

  m_gpuProgramManager.reset();
}

void FrontendRenderer::AddUserEvent(UserEvent const & event)
{
  m_userEventStream.AddEvent(event);
  if (IsInInfinityWaiting())
    CancelMessageWaiting();
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

void FrontendRenderer::ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero)
{
  ScreenBase const & screen = m_userEventStream.GetCurrentScreen();
  double offset = (screen.PtoG(screen.PixelRect().Center()) - screen.PtoG(pxZero)).Length();
  m2::PointD viewVector = userPos.Move(1.0, -azimuth + math::pi2) - userPos;
  viewVector.Normalize();

  AddUserEvent(SetAnyRectEvent(m2::AnyRectD(userPos + (viewVector * offset), -azimuth,
                                            screen.GlobalRect().GetLocalRect()), true /* animate */));
}

ScreenBase const & FrontendRenderer::UpdateScene(bool & modelViewChanged)
{
  bool viewportChanged;
  ScreenBase const & modelView = m_userEventStream.ProcessEvents(modelViewChanged, viewportChanged);
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
    EmitModelViewChanged(modelView);
  }

  return modelView;
}

void FrontendRenderer::EmitModelViewChanged(ScreenBase const & modelView) const
{
  m_modelViewChangedFn(modelView);
}

} // namespace df
