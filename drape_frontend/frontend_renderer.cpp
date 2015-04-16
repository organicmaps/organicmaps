#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

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

FrontendRenderer::FrontendRenderer(ref_ptr<ThreadsCommutator> commutator,
                                   ref_ptr<dp::OGLContextFactory> oglcontextfactory,
                                   ref_ptr<dp::TextureManager> textureManager,
                                   Viewport viewport)
  : BaseRenderer(ThreadsCommutator::RenderThread, commutator, oglcontextfactory)
  , m_textureManager(textureManager)
  , m_gpuProgramManager(new dp::GpuProgramManager())
  , m_viewport(viewport)
  , m_tileTree(new TileTree())
{
#ifdef DRAW_INFO
  m_tpf = 0,0;
  m_fps = 0.0;
#endif

  RefreshProjection();
  RefreshModelView();

  m_tileTree->SetHandlers(bind(&FrontendRenderer::OnAddRenderGroup, this, _1, _2, _3),
                          bind(&FrontendRenderer::OnDeferRenderGroup, this, _1, _2, _3),
                          bind(&FrontendRenderer::OnActivateTile, this, _1),
                          bind(&FrontendRenderer::OnRemoveTile, this, _1));
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
      ref_ptr<FlushRenderBucketMessage> msg = df::CastMessage<FlushRenderBucketMessage>(message);
      dp::GLState const & state = msg->GetState();
      TileKey const & key = msg->GetKey();
      drape_ptr<dp::RenderBucket> bucket = move(msg->AcceptBuffer());
      ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
      program->Bind();
      bucket->GetBuffer()->Build(program);
      if (!IsUserMarkLayer(key))
        m_tileTree->ProcessTile(key, GetCurrentZoomLevel(), state, move(bucket));
      else
        m_userMarkRenderGroups.emplace_back(new UserMarkRenderGroup(state, key, move(bucket)));
      break;
    }

  case Message::FinishReading:
    {
      ref_ptr<FinishReadingMessage> msg = df::CastMessage<FinishReadingMessage>(message);
      m_tileTree->FinishTiles(msg->GetTiles(), GetCurrentZoomLevel());
      break;
    }

  case Message::Resize:
    {
      ref_ptr<ResizeMessage> rszMsg = df::CastMessage<ResizeMessage>(message);
      m_viewport = rszMsg->GetViewport();
      m_view.OnSize(m_viewport.GetX0(), m_viewport.GetY0(),
                    m_viewport.GetWidth(), m_viewport.GetHeight());
      m_contextFactory->getDrawContext()->resize(m_viewport.GetWidth(), m_viewport.GetHeight());
      RefreshProjection();
      RefreshModelView();
      ResolveTileKeys();

      TTilesCollection tiles;
      m_tileTree->GetTilesCollection(tiles, GetCurrentZoomLevel());

      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<ResizeMessage>(m_viewport),
                                MessagePriority::Normal);
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<UpdateReadManagerMessage>(m_view, move(tiles)),
                                MessagePriority::Normal);
      break;
    }

  case Message::MyPositionShape:
    m_myPositionMark = CastMessage<MyPositionShapeMessage>(message)->AcceptShape();
    break;

  case Message::InvalidateRect:
    {
      // TODO(@kuznetsov): implement invalidation

      //InvalidateRectMessage * m = df::CastMessage<InvalidateRectMessage>(message);
      //TTilesCollection keyStorage;
      //Message * msgToBackend = new InvalidateReadManagerRectMessage(keyStorage);
      //m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
      //                          dp::MovePointer(msgToBackend),
      //                          MessagePriority::Normal);
      break;
    }

  case Message::ClearUserMarkLayer:
    {
      TileKey const & tileKey = df::CastMessage<ClearUserMarkLayerMessage>(message)->GetKey();
      auto const functor = [&tileKey](unique_ptr<UserMarkRenderGroup> const & g)
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
      ref_ptr<ChangeUserMarkLayerVisibilityMessage> m = df::CastMessage<ChangeUserMarkLayerVisibilityMessage>(message);
      TileKey const & key = m->GetKey();
      if (m->IsVisible())
        m_userMarkVisibility.insert(key);
      else
        m_userMarkVisibility.erase(key);
      break;
    }
  case Message::GuiLayerRecached:
    {
      ref_ptr<GuiLayerRecachedMessage> msg = df::CastMessage<GuiLayerRecachedMessage>(message);
      drape_ptr<gui::LayerRenderer> renderer = move(msg->AcceptRenderer());
      renderer->Build(make_ref<dp::GpuProgramManager>(m_gpuProgramManager));
      if (m_guiRenderer == nullptr)
        m_guiRenderer = move(renderer);
      else
        m_guiRenderer->Merge(make_ref<gui::LayerRenderer>(renderer));
      break;
    }
  case Message::StopRendering:
    {
      ProcessStopRenderingMessage();
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

void FrontendRenderer::AddToRenderGroup(vector<unique_ptr<RenderGroup>> & groups,
                                        dp::GLState const & state,
                                        drape_ptr<dp::RenderBucket> && renderBucket,
                                        TileKey const & newTile)
{
  unique_ptr<RenderGroup> group(new RenderGroup(state, newTile));
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
      group->DeleteLater();
  }

  auto removePredicate = [&tileKey](unique_ptr<RenderGroup> const & group)
  {
    return group->GetTileKey() == tileKey;
  };
  m_deferredRenderGroups.erase(remove_if(m_deferredRenderGroups.begin(),
                                         m_deferredRenderGroups.end(),
                                         removePredicate),
                               m_deferredRenderGroups.end());
}

void FrontendRenderer::RenderScene()
{
#ifdef DRAW_INFO
  BeforeDrawFrame();
#endif

  RenderGroupComparator comparator;
  sort(m_renderGroups.begin(), m_renderGroups.end(), bind(&RenderGroupComparator::operator (), &comparator, _1, _2));

  m_overlayTree.StartOverlayPlacing(m_view);
  size_t eraseCount = 0;
  for (size_t i = 0; i < m_renderGroups.size(); ++i)
  {
    unique_ptr<RenderGroup> & group = m_renderGroups[i];
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
      group->CollectOverlay(make_ref(&m_overlayTree));
      break;
    case dp::GLState::DynamicGeometry:
      group->Update(m_view);
      break;
    default:
      break;
    }
  }
  m_overlayTree.EndOverlayPlacing();
  m_renderGroups.resize(m_renderGroups.size() - eraseCount);

  m_viewport.Apply();
  GLFunctions::glClear();

  dp::GLState::DepthLayer prevLayer = dp::GLState::GeometryLayer;
  for (unique_ptr<RenderGroup> const & group : m_renderGroups)
  {
    dp::GLState const & state = group->GetState();
    dp::GLState::DepthLayer layer = state.GetDepthLayer();
    if (prevLayer != layer && layer == dp::GLState::OverlayLayer)
    {
      GLFunctions::glClearDepth();
      if (m_myPositionMark != nullptr)
        m_myPositionMark->Render(m_view, make_ref<dp::GpuProgramManager>(m_gpuProgramManager), m_generalUniforms);
    }

    prevLayer = layer;
    ASSERT_LESS_OR_EQUAL(prevLayer, layer, ());

    ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
    program->Bind();
    ApplyUniforms(m_generalUniforms, program);
    ApplyState(state, program);

    group->Render(m_view);
  }

  GLFunctions::glClearDepth();

  for (unique_ptr<UserMarkRenderGroup> const & group : m_userMarkRenderGroups)
  {
    ASSERT(group.get() != nullptr, ());
    if (m_userMarkVisibility.find(group->GetTileKey()) != m_userMarkVisibility.end())
    {
      dp::GLState const & state = group->GetState();
      ref_ptr<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
      program->Bind();
      ApplyUniforms(m_generalUniforms, program);
      ApplyState(state, program);
      group->Render(m_view);
    }
  }

  GLFunctions::glClearDepth();
  if (m_guiRenderer != nullptr)
    m_guiRenderer->Render(make_ref<dp::GpuProgramManager>(m_gpuProgramManager), m_view);

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

void FrontendRenderer::RefreshModelView()
{
  ScreenBase::MatrixT const & m = m_view.GtoPMatrix();
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
  //TODO(@kuznetsov): revise it
  int const upperScale = scales::GetUpperScale();
  int const zoomLevel = GetDrawTileScale(m_view);
  return(zoomLevel <= upperScale ? zoomLevel : upperScale);
}

void FrontendRenderer::ResolveTileKeys()
{
  ResolveTileKeys(GetCurrentZoomLevel());
}

void FrontendRenderer::ResolveTileKeys(int tileScale)
{
  // equal for x and y
  double const range = MercatorBounds::maxX - MercatorBounds::minX;
  double const rectSize = range / (1 << tileScale);

  m2::RectD const & clipRect = m_view.ClipRect();

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
        m_tileTree->RequestTile(key);
    }
  }
  m_tileTree->EndRequesting();
}

FrontendRenderer::Routine::Routine(FrontendRenderer & renderer) : m_renderer(renderer) {}

void FrontendRenderer::Routine::Do()
{
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

  my::Timer timer;
  //double processingTime = InitAvarageTimePerMessage; // By init we think that one message processed by 1ms

  timer.Reset();
  while (!IsCancelled())
  {
    context->setDefaultFramebuffer();
    m_renderer.m_textureManager->UpdateDynamicTextures();
    m_renderer.RenderScene();
    m_renderer.UpdateScene();

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

    context->present();
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
  m_myPositionMark.reset();

  m_gpuProgramManager.reset();
}

void FrontendRenderer::SetModelView(ScreenBase const & screen)
{
  lock_guard<mutex> lock(m_modelViewMutex);
  m_newView = screen;
}

void FrontendRenderer::UpdateScene()
{
  lock_guard<mutex> lock(m_modelViewMutex);
  if (m_view != m_newView)
  {
    m_view = m_newView;
    RefreshModelView();
    ResolveTileKeys();

    TTilesCollection tiles;
    m_tileTree->GetTilesCollection(tiles, GetCurrentZoomLevel());

    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                              make_unique_dp<UpdateReadManagerMessage>(m_view, move(tiles)),
                              MessagePriority::Normal);
  }
}

} // namespace df
