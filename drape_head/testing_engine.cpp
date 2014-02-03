#include "testing_engine.hpp"

#include "../drape_frontend/vizualization_params.hpp"
#include "../drape_frontend/line_shape.hpp"

#include "../base/stl_add.hpp"

#include "../std/bind.hpp"

namespace df
{
  TestingEngine::TestingEngine(RefPointer<OGLContextFactory> oglcontextfactory,
                               double vs, const df::Viewport & viewport)
    : m_contextFactory(oglcontextfactory)
    , m_viewport(viewport)
  {
    GLFunctions::Init();
    df::VizualizationParams::SetVisualScale(vs);
    m_contextFactory->getDrawContext()->makeCurrent();

    m_batcher.Reset(new Batcher());
    m_programManager.Reset(new GpuProgramManager());

    ModelViewInit();
    ProjectionInit();
  }

  TestingEngine::~TestingEngine()
  {
    ClearScene();
    m_batcher.Destroy();
    m_programManager.Destroy();
  }

  void TestingEngine::Draw()
  {
    ClearScene();
    m_batcher->StartSession(bind(&df::TestingEngine::OnFlushData, this, _1, _2));
    DrawImpl();
    m_batcher->EndSession();

    OGLContext * context = m_contextFactory->getDrawContext();
    context->setDefaultFramebuffer();

    m_viewport.Apply();
    GLFunctions::glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
    GLFunctions::glClear();

    scene_t::iterator it = m_scene.begin();
    for(; it != m_scene.end(); ++it)
    {
      const GLState & state = it->first;
      RefPointer<GpuProgram> prg = m_programManager->GetProgram(state.GetProgramIndex());
      prg->Bind();
      ApplyState(state, prg);
      ApplyUniforms(m_generalUniforms, prg);

      it->second->Render();
    }

    context->present();
  }

  void TestingEngine::OnSizeChanged(int x0, int y0, int w, int h)
  {
    m_viewport.SetViewport(x0, y0, w, h);
    ModelViewInit();
    ProjectionInit();
    Draw();
  }

  void TestingEngine::SetAngle(float radians)
  {
    ModelViewInit();
    ProjectionInit();

    Draw();
  }

  void TestingEngine::DrawImpl()
  {
    // Insert shape batchering here
    float left = m_viewport.GetX0();
    float right = left + m_viewport.GetWidth();
    float bottom = m_viewport.GetY0();
    float top = bottom + m_viewport.GetHeight();

    vector<m2::PointF> linePoints;
    linePoints.push_back(m2::PointF(left + 40.0f, bottom + 40.0f));
    linePoints.push_back(m2::PointF(right - 40.0f, bottom + 40.0f));
    linePoints.push_back(m2::PointF(right - 40.0f, top));
    linePoints.push_back(m2::PointF(left + 40.0f, top));
    linePoints.push_back(m2::PointF(left + 40.0f, bottom + 40.0f));

    LineViewParams params;
    params.m_color = Color(255, 50, 50, 255);
    params.m_width = 3.5f;
    df::LineShape * line = new df::LineShape(linePoints, 0.0f, params);
    line->Draw(m_batcher.GetRefPointer());
  }

  void TestingEngine::ModelViewInit()
  {
    float modelView[4 * 4] =
    {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0
    };

    m_generalUniforms.SetMatrix4x4Value("modelView", modelView);
  }

  void TestingEngine::ProjectionInit()
  {
    float left = m_viewport.GetX0();
    float right = left + m_viewport.GetWidth();
    float bottom = m_viewport.GetY0();
    float top = bottom + m_viewport.GetHeight();
    float near = -20000.0f;
    float far = 20000.0f;

    float m[4 * 4];
    memset(m, 0, sizeof(m));
    m[0]  = 2.0f / (right - left);
    m[3]  = - (right + left) / (right - left);
    m[5]  = 2.0f / (top - bottom);
    m[7]  = - (top + bottom) / (top - bottom);
    m[10] = -2.0f / (far - near);
    m[11] = - (far + near) / (far - near);
    m[15] = 1.0;

    m_generalUniforms.SetMatrix4x4Value("projection", m);
  }

  void TestingEngine::OnFlushData(const GLState & state, TransferPointer<VertexArrayBuffer> vao)
  {
    MasterPointer<VertexArrayBuffer> vaoMaster(vao);
    vaoMaster->Build(m_programManager->GetProgram(state.GetProgramIndex()));
    m_scene.insert(make_pair(state, vaoMaster));
  }

  void TestingEngine::ClearScene()
  {
    GetRangeDeletor(m_scene, MasterPointerDeleter())();
  }
}
