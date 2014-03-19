#include "testing_engine.hpp"

#include "../drape_frontend/visual_params.hpp"
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
    df::VisualParams::Init(vs, df::CalculateTileSize(viewport.GetWidth(), viewport.GetHeight()));
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

    const float resolution = 32;
    const float stepX = right/resolution;
    const float stepY = top/resolution;
    m2::PointF grid[32][32]; // to simpify testing
    for (size_t i = 1; i <= resolution; ++i)
      for (size_t j = 1; j <= resolution; ++j)
        grid[i-1][j-1] = m2::PointF(stepX*i, stepY*j);

    // grid:
    // [31,31] ... [31,31]
    //  ...         ...
    // [0,0]   ... [0,31]

    //1
    vector<m2::PointF> linePoints1;

    linePoints1.push_back(grid[1][1]);
    linePoints1.push_back(grid[4][6]);
    linePoints1.push_back(grid[8][1]);
    linePoints1.push_back(grid[16][10]);
    linePoints1.push_back(grid[24][1]);
    linePoints1.push_back(grid[29][4]);

    LineViewParams params1;
    params1.m_cap   = RoundCap;
    params1.m_join  = RoundJoin;
    params1.m_color = Color(255, 255, 50, 255);
    params1.m_width = 80.f;
    df::LineShape * line1 = new df::LineShape(linePoints1, 0.0f, params1);
    line1->Draw(m_batcher.GetRefPointer(), MakeStackRefPointer<TextureManager>(NULL));
    //

    //2
    vector<m2::PointF> linePoints2;

    linePoints2.push_back(grid[2][28]);
    linePoints2.push_back(grid[6][28]);
    linePoints2.push_back(grid[6][22]);
    linePoints2.push_back(grid[2][22]);
    linePoints2.push_back(grid[2][16]);
    linePoints2.push_back(grid[6][16]);

    LineViewParams params2;
    params2.m_cap   = SquareCap;
    params2.m_join  = RoundJoin;
    params2.m_color = Color(0, 255, 255, 255);
    params2.m_width = 50.f;
    df::LineShape * line2 = new df::LineShape(linePoints2, 0.0f, params2);
    line2->Draw(m_batcher.GetRefPointer(), MakeStackRefPointer<TextureManager>(NULL));
    //

    //3
    vector<m2::PointF> linePoints3;

    linePoints3.push_back(grid[1][9]);
    linePoints3.push_back(grid[4][10]);
    linePoints3.push_back(grid[8][9]);
    linePoints3.push_back(grid[16][12]);
    linePoints3.push_back(grid[24][9]);
    linePoints3.push_back(grid[29][12]);

    LineViewParams params3;
    params3.m_cap   = ButtCap;
    params3.m_join  = MiterJoin;
    params3.m_color = Color(255, 0, 255, 255);
    params3.m_width = 60.f;
    df::LineShape * line3 = new df::LineShape(linePoints3, 0.0f, params3);
    line3->Draw(m_batcher.GetRefPointer(), MakeStackRefPointer<TextureManager>(NULL));
    //
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
