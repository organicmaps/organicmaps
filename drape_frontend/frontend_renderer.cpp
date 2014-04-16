#include "frontend_renderer.hpp"
#include "message_subclasses.hpp"
#include "visual_params.hpp"

#include "../base/timer.hpp"
#include "../base/assert.hpp"
#include "../base/stl_add.hpp"

#include "../geometry/any_rect2d.hpp"

#include "../std/bind.hpp"
#include "../std/cmath.hpp"

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

void OrthoMatrix(float * m, float left, float right, float bottom, float top, float near, float far)
{
  memset(m, 0, 16 * sizeof(float));
  m[0]  = 2.0f / (right - left);
  m[3]  = - (right + left) / (right - left);
  m[5]  = 2.0f / (top - bottom);
  m[7]  = - (top + bottom) / (top - bottom);
  m[10] = -2.0f / (far - near);
  m[11] = - (far + near) / (far - near);
  m[15] = 1.0;
}

} // namespace

FrontendRenderer::FrontendRenderer(RefPointer<ThreadsCommutator> commutator,
                                   RefPointer<OGLContextFactory> oglcontextfactory,
                                   TransferPointer<TextureSetController> textureController,
                                   Viewport viewport)
  : m_commutator(commutator)
  , m_contextFactory(oglcontextfactory)
  , m_textureController(textureController)
  , m_gpuProgramManager(new GpuProgramManager())
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

void FrontendRenderer::AcceptMessage(RefPointer<Message> message)
{
  switch (message->GetType())
  {
  case Message::FlushTile:
    {
      FlushRenderBucketMessage * msg = static_cast<FlushRenderBucketMessage *>(message.GetRaw());
      const GLState & state = msg->GetState();
      const TileKey & key = msg->GetKey();
      MasterPointer<RenderBucket> bucket(msg->AcceptBuffer());
      RefPointer<GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
      program->Bind();
      bucket->GetBuffer()->Build(program);
      RenderGroup * group = new RenderGroup(state, key);
      group->AddBucket(bucket.Move());
      m_renderGroups.push_back(group);
      break;
    }

  case Message::Resize:
    {
      ResizeMessage * rszMsg = static_cast<ResizeMessage *>(message.GetRaw());
      m_viewport = rszMsg->GetViewport();
      m_view.OnSize(0, 0, m_viewport.GetWidth(), m_viewport.GetHeight());
      RefreshProjection();
      RefreshModelView();
      ResolveTileKeys();
      ASSERT(m_tiles != NULL, ());
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                MovePointer<Message>(new UpdateReadManagerMessage(m_view, m_tiles)));
      break;
    }

  case Message::UpdateModelView:
    {
      UpdateModelViewMessage * coverMessage = static_cast<UpdateModelViewMessage *>(message.GetRaw());
      m_view = coverMessage->GetScreen();
      RefreshModelView();
      ResolveTileKeys();
      ASSERT(m_tiles != NULL, ());
      m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                MovePointer<Message>(new UpdateReadManagerMessage(m_view, m_tiles)));
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

  RenderBucketComparator comparator(GetTileKeyStorage());
  sort(m_renderGroups.begin(), m_renderGroups.end(), bind(&RenderBucketComparator::operator (), &comparator, _1, _2));

  m_overlayTree.StartOverlayPlacing(m_view);
  size_t eraseCount = 0;
  for (size_t i = 0; i < m_renderGroups.size(); ++i)
  {
    if (m_renderGroups[i]->IsEmpty())
      continue;

    if (m_renderGroups[i]->IsPendingOnDelete())
    {
      delete m_renderGroups[i];
      ++eraseCount;
      continue;
    }

    if (m_renderGroups[i]->GetState().GetDepthLayer() == GLState::OverlayLayer)
      m_renderGroups[i]->CollectOverlay(MakeStackRefPointer(&m_overlayTree));
  }
  m_overlayTree.EndOverlayPlacing();
  m_renderGroups.resize(m_renderGroups.size() - eraseCount);

  m_viewport.Apply();
  GLFunctions::glEnable(gl_const::GLDepthTest);

  GLFunctions::glClearColor(0.93f, 0.93f, 0.86f, 1.f);
  GLFunctions::glClearDepth(1.0);
  GLFunctions::glDepthFunc(gl_const::GLLessOrEqual);
  GLFunctions::glDepthMask(true);

  GLFunctions::glClear();

  for (size_t i = 0; i < m_renderGroups.size(); ++i)
  {
    RenderGroup * group = m_renderGroups[i];
    GLState const & state = group->GetState();
    RefPointer<GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
    program->Bind();
    ApplyUniforms(m_generalUniforms, program);
    ApplyState(state, program, m_textureController.GetRefPointer());

    group->Render();
  }

#ifdef DRAW_INFO
  AfterDrawFrame();
#endif
}

void FrontendRenderer::RefreshProjection()
{
  float m[4*4];

  OrthoMatrix(m, 0.0f, m_viewport.GetWidth(), m_viewport.GetHeight(), 0.0f, -20000.0f, 20000.0f);
  m_generalUniforms.SetMatrix4x4Value("projection", m);
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
  set<TileKey> & tiles = GetTileKeyStorage();
  tiles.clear();

  int const tileScale = df::GetTileScaleBase(m_view);
  // equal for x and y
  double const range = MercatorBounds::maxX - MercatorBounds::minX;
  double const rectSize = range / (1 << tileScale);

  m2::RectD const & clipRect   = m_view.ClipRect();

  int const minTileX = static_cast<int>(floor(clipRect.minX() / rectSize));
  int const maxTileX = static_cast<int>(ceil(clipRect.maxX() / rectSize));
  int const minTileY = static_cast<int>(floor(clipRect.minY() / rectSize));
  int const maxTileY = static_cast<int>(ceil(clipRect.maxY() / rectSize));

  for (int tileY = minTileY; tileY < maxTileY; ++tileY)
  {
    for (int tileX = minTileX; tileX < maxTileX; ++tileX)
      tiles.insert(TileKey(tileX, tileY, tileScale));
  }
}

set<TileKey> & FrontendRenderer::GetTileKeyStorage()
{
  if (m_tiles == NULL)
    m_tiles.reset(new set<TileKey>());

  return *m_tiles;
}

void FrontendRenderer::StartThread()
{
  m_selfThread.Create(this);
}

void FrontendRenderer::StopThread()
{
  IRoutine::Cancel();
  CloseQueue();
  m_selfThread.Join();
}

void FrontendRenderer::ThreadMain()
{
  OGLContext * context = m_contextFactory->getDrawContext();
  context->makeCurrent();

  my::Timer timer;
  //double processingTime = InitAvarageTimePerMessage; // By init we think that one message processed by 1ms

  timer.Reset();
  while (!IsCancelled())
  {
    context->setDefaultFramebuffer();
    RenderScene();

    //double avarageMessageTime = processingTime;
    //processingTime = timer.ElapsedSeconds();
    //int messageCount = 0;
    double availableTime = VSyncInterval - (timer.ElapsedSeconds() /*+ avarageMessageTime*/);

    while (availableTime > 0)
    {
      ProcessSingleMessage(availableTime * 1000.0);
      availableTime = VSyncInterval - (timer.ElapsedSeconds() /*+ avarageMessageTime*/);
      //messageCount++;
    }

    //processingTime = (timer.ElapsedSeconds() - processingTime) / messageCount;

    context->present();
    timer.Reset();
  }

  ReleaseResources();
}

void FrontendRenderer::ReleaseResources()
{
  DeleteRenderData();
  m_gpuProgramManager.Destroy();
  m_textureController.Destroy();
}

void FrontendRenderer::Do()
{
  ThreadMain();
}

void FrontendRenderer::DeleteRenderData()
{
  (void)GetRangeDeletor(m_renderGroups, DeleteFunctor())();
}

} // namespace df
