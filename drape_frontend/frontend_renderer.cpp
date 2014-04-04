#include "frontend_renderer.hpp"
#include "message_subclasses.hpp"

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
    const double InitAvarageTimePerMessage = 0.003;
#else
    const double VSyncInterval = 0.014;
    const double InitAvarageTimePerMessage = 0.001;
#endif
  }

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
        MasterPointer<RenderBucket> buffer(msg->AcceptBuffer());
        RefPointer<GpuProgram> program = m_gpuProgramManager->GetProgram(state.GetProgramIndex());
        program->Bind();
        buffer->GetBuffer()->Build(program);
        render_data_t::iterator renderIterator = m_renderData.insert(make_pair(state, buffer));
        m_tileData.insert(make_pair(key, renderIterator));
        break;
      }
    case Message::DropTiles:
      {
        CoverageUpdateDescriptor const & descr = static_cast<DropTilesMessage *>(message.GetRaw())->GetDescriptor();
        ASSERT(!descr.IsEmpty(), ());

        if (!descr.IsDropAll())
        {
          vector<TileKey> const & tilesToDrop = descr.GetTilesToDrop();
          for (size_t i = 0; i < tilesToDrop.size(); ++i)
          {
            tile_data_range_t range = m_tileData.equal_range(tilesToDrop[i]);
            for (tile_data_iter eraseIter = range.first; eraseIter != range.second; ++eraseIter)
            {
              eraseIter->second->second.Destroy();
              m_renderData.erase(eraseIter->second);
            }
            m_tileData.erase(range.first, range.second);
          }
        }
        else
          DeleteRenderData();

        break;
      }

    case Message::Resize:
      {
        ResizeMessage * rszMsg = static_cast<ResizeMessage *>(message.GetRaw());
        m_viewport = rszMsg->GetViewport();
        RefreshProjection();
        RefreshModelView();
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  MovePointer<Message>(new UpdateCoverageMessage(m_view)));
        break;
      }

    case Message::UpdateCoverage:
      {
        UpdateCoverageMessage * coverMessage = static_cast<UpdateCoverageMessage *>(message.GetRaw());
        m_view = coverMessage->GetScreen();
        RefreshModelView();
        m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  MovePointer<Message>(new UpdateCoverageMessage(m_view)));
        break;
      }

    case Message::Rotate:
      {
        //RotateMessage * rtMsg = static_cast<RotateMessage *>(message.GetRaw());
        //m_modelView.Rotate(rtMsg->GetDstAngle());
        //RefreshModelView();

        //m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
        //                          MovePointer<Message>(new UpdateCoverageMessage(m_modelView)));
        break;
      }

    default:
      ASSERT(false, ());
    }
  }

  namespace
  {
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
  }

  void FrontendRenderer::RenderScene()
  {
#ifdef DRAW_INFO
    BeforeDrawFrame();
#endif

    m_viewport.Apply();
    GLFunctions::glEnable(GLConst::GLDepthTest);

    GLFunctions::glClearColor(0.93f, 0.93f, 0.86f, 1.f);
    GLFunctions::glClearDepth(1.0);
    GLFunctions::glDepthFunc(GLConst::GLLessOrEqual);
    GLFunctions::glDepthMask(true);

    GLFunctions::glClear();
    for_each(m_renderData.begin(), m_renderData.end(), bind(&FrontendRenderer::RenderPartImpl, this, _1));

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

  void FrontendRenderer::RenderPartImpl(pair<const GLState, MasterPointer<RenderBucket> > & node)
  {
    RefPointer<GpuProgram> program = m_gpuProgramManager->GetProgram(node.first.GetProgramIndex());

    program->Bind();
    ApplyState(node.first, program, m_textureController.GetRefPointer());
    ApplyUniforms(m_generalUniforms, program);

    node.second->Render();
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
    double processingTime = InitAvarageTimePerMessage; // By init we think that one message processed by 1ms

    timer.Reset();
    while (!IsCancelled())
    {
      context->setDefaultFramebuffer();
      RenderScene();

      double avarageMessageTime = processingTime;
      processingTime = timer.ElapsedSeconds();
      int messageCount = 0;

      while (timer.ElapsedSeconds() + avarageMessageTime < VSyncInterval)
      {
        ProcessSingleMessage(false);
        messageCount++;
      }

      if (messageCount == 0)
      {
        ProcessSingleMessage(false);
        messageCount++;
      }

      processingTime = (timer.ElapsedSeconds() - processingTime) / messageCount;

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
    m_tileData.clear();
    GetRangeDeletor(m_renderData, MasterPointerDeleter())();
  }
}
