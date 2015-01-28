#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/utils/projection.hpp"

#include "geometry/any_rect2d.hpp"

#include "base/timer.hpp"
#include "base/assert.hpp"
#include "base/stl_add.hpp"

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

FrontendRenderer::FrontendRenderer(dp::RefPointer<ThreadsCommutator> commutator,
                                   dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
                                   Viewport viewport)
  : m_commutator(commutator)
  , m_contextFactory(oglcontextfactory)
  , m_gpuProgramManager(new dp::GpuProgramManager())
  , m_viewport(viewport)
{
#ifdef DRAW_INFO
  m_tpf = 0,0;
  m_fps = 0.0;
#endif

  m_commutator->RegisterThread(ThreadsCommutator::RenderThread, this);

  RefreshProjection();
  RefreshModelView();
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
  }
}
#endif

UserMarkRenderGroup * FrontendRenderer::FindUserMarkRenderGroup(TileKey const & tileKey, bool createIfNeed)
{
  auto it = find_if(m_userMarkRenderGroups.begin(), m_userMarkRenderGroups.end(), [&tileKey](UserMarkRenderGroup * g)
  {
    return g->GetTileKey() == tileKey;
  });

  if (it != m_userMarkRenderGroups.end())
  {
    ASSERT((*it) != nullptr, ());
    return *it;
  }

  if (createIfNeed)
  {
    UserMarkRenderGroup * group = new UserMarkRenderGroup(dp::GLState(0, dp::GLState::UserMarkLayer), tileKey);
    m_userMarkRenderGroups.push_back(group);
    return group;
  }

  return nullptr;
}

void FrontendRenderer::AcceptMessage(dp::RefPointer<Message> message)
{
  switch (message->GetType())
  {
  case Message::FlushTile:
    {
      FlushRenderBucketMessage * msg = df::CastMessage<FlushRenderBucketMessage>(message);
      dp::GLState const & state = msg->GetState();
      TileKey const & key = msg->GetKey();
      dp::MasterPointer<dp::RenderBucket> bucket(msg->AcceptBuffer());
      dp::RefPointer<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
      program->Bind();
      bucket->GetBuffer()->Build(program);
      if (!IsUserMarkLayer(key))
      {
        RenderGroup * group = new RenderGroup(state, key);
        group->AddBucket(bucket.Move());
        m_renderGroups.push_back(group);
      }
      else
      {
        UserMarkRenderGroup * group = FindUserMarkRenderGroup(key, true);
        ASSERT(group != nullptr, ());
        group->SetRenderBucket(state, bucket.Move());
      }
      break;
    }

  case Message::Resize:
    {
      ResizeMessage * rszMsg = df::CastMessage<ResizeMessage>(message);
      m_viewport = rszMsg->GetViewport();
      m_view.OnSize(m_viewport.GetX0(), m_viewport.GetY0(),
                    m_viewport.GetWidth(), m_viewport.GetHeight());
      m_contextFactory->getDrawContext()->resize(m_viewport.GetWidth(), m_viewport.GetHeight());
      RefreshProjection();
      RefreshModelView();
      ResolveTileKeys();
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                dp::MovePointer<Message>(new UpdateReadManagerMessage(m_view, m_tiles)));
      break;
    }

  case Message::UpdateModelView:
    {
      UpdateModelViewMessage * coverMessage = df::CastMessage<UpdateModelViewMessage>(message);
      m_view = coverMessage->GetScreen();
      RefreshModelView();
      ResolveTileKeys();
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                dp::MovePointer<Message>(new UpdateReadManagerMessage(m_view, m_tiles)));
      break;
    }

  case Message::InvalidateRect:
    {
      InvalidateRectMessage * m = df::CastMessage<InvalidateRectMessage>(message);

      set<TileKey> keyStorage;
      ResolveTileKeys(keyStorage, m->GetRect());
      InvalidateRenderGroups(keyStorage);

      Message * msgToBackend = new InvalidateReadManagerRectMessage(keyStorage);
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, dp::MovePointer(msgToBackend));
      break;
    }

  case Message::ClearUserMarkLayer:
    {
      TileKey const & tileKey = df::CastMessage<ClearUserMarkLayerMessage>(message)->GetKey();
      auto it = find_if(m_userMarkRenderGroups.begin(), m_userMarkRenderGroups.end(), [&tileKey](UserMarkRenderGroup * g)
      {
        return g->GetTileKey() == tileKey;
      });

      if (it != m_userMarkRenderGroups.end())
      {
        UserMarkRenderGroup * group = *it;
        ASSERT(group != nullptr, ());
        m_userMarkRenderGroups.erase(it);
        delete group;
      }

      break;
    }
  case Message::ChangeUserMarkLayerVisibility:
    {
      ChangeUserMarkLayerVisibilityMessage * m = df::CastMessage<ChangeUserMarkLayerVisibilityMessage>(message);
      UserMarkRenderGroup * group = FindUserMarkRenderGroup(m->GetKey(), true);
      ASSERT(group != nullptr, ());
      group->SetIsVisible(m->IsVisible());
      break;
    }

  default:
    ASSERT(false, ());
  }
}

void FrontendRenderer::RenderScene()
{
#ifdef DRAW_INFO
  BeforeDrawFrame();
#endif

  RenderGroupComparator comparator(GetTileKeyStorage());
  sort(m_renderGroups.begin(), m_renderGroups.end(), bind(&RenderGroupComparator::operator (), &comparator, _1, _2));

  m_overlayTree.StartOverlayPlacing(m_view);
  size_t eraseCount = 0;
  for (size_t i = 0; i < m_renderGroups.size(); ++i)
  {
    RenderGroup * group = m_renderGroups[i];
    if (group->IsEmpty())
      continue;

    if (group->IsPendingOnDelete())
    {
      delete group;
      ++eraseCount;
      continue;
    }

    switch (group->GetState().GetDepthLayer())
    {
    case dp::GLState::OverlayLayer:
      group->CollectOverlay(dp::MakeStackRefPointer(&m_overlayTree));
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
  GLFunctions::glEnable(gl_const::GLDepthTest);

  GLFunctions::glClearColor(0.93f, 0.93f, 0.86f, 1.f);
  GLFunctions::glClearDepthValue(1.0);
  GLFunctions::glDepthFunc(gl_const::GLLessOrEqual);
  GLFunctions::glDepthMask(true);

  GLFunctions::glClear();

  dp::GLState::DepthLayer prevLayer = dp::GLState::GeometryLayer;
  for (size_t i = 0; i < m_renderGroups.size(); ++i)
  {
    RenderGroup * group = m_renderGroups[i];
    dp::GLState const & state = group->GetState();
    dp::GLState::DepthLayer layer = state.GetDepthLayer();
    if (prevLayer != layer && layer == dp::GLState::OverlayLayer)
      GLFunctions::glClearDepth();

    prevLayer = layer;
    ASSERT_LESS_OR_EQUAL(prevLayer, layer, ());

    dp::RefPointer<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
    program->Bind();
    ApplyUniforms(m_generalUniforms, program);
    ApplyState(state, program);

    group->Render(m_view);
  }

  GLFunctions::glClearDepth();

  for (UserMarkRenderGroup * group : m_userMarkRenderGroups)
  {
    ASSERT(group != nullptr, ());
    if (group->IsVisible())
    {
      dp::GLState const & state = group->GetState();
      dp::RefPointer<dp::GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
      program->Bind();
      ApplyUniforms(m_generalUniforms, program);
      ApplyState(state, program);
      group->Render(m_view);
    }
  }

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

void FrontendRenderer::ResolveTileKeys()
{
  ResolveTileKeys(GetTileKeyStorage(), df::GetTileScaleBase(m_view));
}

void FrontendRenderer::ResolveTileKeys(set<TileKey> & keyStorage, m2::RectD const & rect)
{
  ResolveTileKeys(keyStorage, df::GetTileScaleBase(rect));
}

void FrontendRenderer::ResolveTileKeys(set<TileKey> & keyStorage, int tileScale)
{
  // equal for x and y
  double const range = MercatorBounds::maxX - MercatorBounds::minX;
  double const rectSize = range / (1 << tileScale);

  m2::RectD const & clipRect   = m_view.ClipRect();

  int const minTileX = static_cast<int>(floor(clipRect.minX() / rectSize));
  int const maxTileX = static_cast<int>(ceil(clipRect.maxX() / rectSize));
  int const minTileY = static_cast<int>(floor(clipRect.minY() / rectSize));
  int const maxTileY = static_cast<int>(ceil(clipRect.maxY() / rectSize));

  keyStorage.clear();
  for (int tileY = minTileY; tileY < maxTileY; ++tileY)
  {
    for (int tileX = minTileX; tileX < maxTileX; ++tileX)
    {
      TileKey key(tileX, tileY, tileScale);
      if (clipRect.IsIntersect(key.GetGlobalRect()))
        keyStorage.insert(key);
    }
  }
}

void FrontendRenderer::InvalidateRenderGroups(set<TileKey> & keyStorage)
{
  for (size_t i = 0; i < m_renderGroups.size(); ++i)
  {
    RenderGroup * group = m_renderGroups[i];
    if (keyStorage.find(group->GetTileKey()) != keyStorage.end())
      group->DeleteLater();
  }
}

set<TileKey> & FrontendRenderer::GetTileKeyStorage()
{
  return m_tiles;
}

void FrontendRenderer::StartThread()
{
  m_selfThread.Create(make_unique<Routine>(*this));
}

void FrontendRenderer::StopThread()
{
  m_selfThread.GetRoutine()->Cancel();
  CloseQueue();
  m_selfThread.Join();
}

FrontendRenderer::Routine::Routine(FrontendRenderer & renderer) : m_renderer(renderer) {}

void FrontendRenderer::Routine::Do()
{
  dp::OGLContext * context = m_renderer.m_contextFactory->getDrawContext();
  context->makeCurrent();
  GLFunctions::Init();

  my::Timer timer;
  //double processingTime = InitAvarageTimePerMessage; // By init we think that one message processed by 1ms

  timer.Reset();
  while (!IsCancelled())
  {
    context->setDefaultFramebuffer();
    m_renderer.RenderScene();

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
  }

  m_renderer.ReleaseResources();
}

void FrontendRenderer::ReleaseResources()
{
  DeleteRenderData();
  m_gpuProgramManager.Destroy();
}

void FrontendRenderer::DeleteRenderData()
{
  DeleteRange(m_renderGroups, DeleteFunctor());
  DeleteRange(m_userMarkRenderGroups, DeleteFunctor());
}

} // namespace df
