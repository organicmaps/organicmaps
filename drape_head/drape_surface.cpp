#include "drape_surface.hpp"

#include "../drape/utils/list_generator.hpp"
#include "../drape/shader_def.hpp"

#include "../base/stl_add.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "../std/cmath.hpp"

namespace
{
  struct Deleter
  {
    void operator()(pair<const GLState, vector<MasterPointer<VertexArrayBuffer> > > & value)
    {
      GetRangeDeletor(value.second, MasterPointerDeleter())();
    }
  };
}

DrapeSurface::DrapeSurface()
  : m_contextFactory(NULL)
{
  setSurfaceType(QSurface::OpenGLSurface);
  m_batcher = new Batcher();
  m_programManager = new GpuProgramManager();

  QObject::connect(this, SIGNAL(heightChanged(int)), this, SLOT(RefreshProjector(int)));
  QObject::connect(this, SIGNAL(widthChanged(int)), this, SLOT(RefreshProjector(int)));
}

DrapeSurface::~DrapeSurface()
{
  GetRangeDeletor(m_frames, Deleter())();
  delete m_batcher;
  delete m_programManager;
  delete m_contextFactory;
}

void DrapeSurface::exposeEvent(QExposeEvent *e)
{
  Q_UNUSED(e);

  if (isExposed())
  {
    if (m_contextFactory == NULL)
    {
      m_contextFactory = new QtOGLContextFactory(this);
      m_contextFactory->getDrawContext()->makeCurrent();
      CreateEngine();
    }

    Render();
    m_contextFactory->getDrawContext()->present();
  }
}

void DrapeSurface::timerEvent(QTimerEvent * e)
{
  if (e->timerId() == m_timerID)
  {
    static const float _2pi = 2 * math::pi;
    static float angle = 0.0;
    angle += 0.035;
    if (angle > _2pi)
      angle -= _2pi;
    RefreshMVUniform(angle);
    Render();
    m_contextFactory->getDrawContext()->present();
  }
}

namespace
{
  void OrthoMatrix(float * m, float left, float right, float bottom, float top, float near, float far)
  {
    memset(m, 0, 16 * sizeof(float));
    m[0]  = 2.0f / (right - left);
    m[4]  = - (right + left) / (right - left);
    m[5]  = 2.0f / (top - bottom);
    m[9]  = - (top + bottom) / (top - bottom);
    m[10] = -2.0f / (far - near);
    m[14] = - (far + near) / (far - near);
    m[15] = 1.0;
  }
}

void DrapeSurface::CreateEngine()
{
  glClearColor(0.8, 0.8, 0.8, 1.0);

  GLFunctions::Init();
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glClearDepth(0.0);

  RefreshMVUniform(0.0);
  RefreshProjector(0);

  UniformValuesStorage uniforms;
  float color[4] = { 0.6, 0.8, 0.3, 1.0 };
  uniforms.SetFloatValue("color", color[0], color[1], color[2], color[3]);

  m_batcher->StartSession(bind(&DrapeSurface::FlushFullBucket, this, _1, _2));
  ListGenerator gen;
  gen.SetDepth(0.5);
  gen.SetProgram(gpu::SOLID_AREA_PROGRAM);
  gen.SetViewport(-1.0f, -1.0f, 2.0f, 2.0f);
  gen.SetUniforms(uniforms);
  gen.Generate(31, *m_batcher);
  m_batcher->EndSession();

  m_timerID = startTimer(1000 / 30);
}

void DrapeSurface::Render()
{
  const qreal retinaScale = devicePixelRatio();
  glViewport(0, 0, width() * retinaScale, height() * retinaScale);
  glClearDepth(1.0);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (!m_frames.empty())
  {
    frames_t::iterator it = m_frames.begin();
    for (; it != m_frames.end(); ++it)
    {
      for (size_t i = 0; i < it->second.size(); ++i)
      {
        RenderBucket(it->first, it->second[i].GetRefPointer());
      }
    }
  }
}

void DrapeSurface::RefreshProjector(int )
{
  int w = width();
  int h = height();
  if (w == 0)
    w = 1;
  float aspect = h / (float)w;
  float m[16];
   if (w > h)
    OrthoMatrix(m, -2.0f / aspect, 2.0f / aspect, -2.0f, 2.0f, -2.0f, 2.0f);
  else
    OrthoMatrix(m, -2.0f, 2.0f, -2.0f * aspect, 2.0f * aspect, -2.0f, 2.0f);

  m_generalUniforms.SetMatrix4x4Value("projection", m);
}

void DrapeSurface::RefreshMVUniform(float angle)
{
  float c = cos(angle);
  float s = sin(angle);
  float model[16] =
  {
    c,  -s,   0.0, 0.0,
    s,   c,   0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  };

  m_generalUniforms.SetMatrix4x4Value("modelView", model);
}

void DrapeSurface::RenderBucket(const GLState & state, RefPointer<VertexArrayBuffer> bucket)
{
  RefPointer<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  ApplyState(state, program);
  ApplyUniforms(m_generalUniforms, program);
  bucket->Render();
}

void DrapeSurface::FlushFullBucket(const GLState & state, TransferPointer<VertexArrayBuffer> bucket)
{
  RefPointer<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  MasterPointer<VertexArrayBuffer> masterBucket(bucket);
  masterBucket->Build(program);
  m_frames[state].push_back(masterBucket);
  Render();
}
