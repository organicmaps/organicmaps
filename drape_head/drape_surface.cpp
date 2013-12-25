#include "drape_surface.hpp"

#include "../drape/utils/list_generator.hpp"
#include "../drape/shader_def.hpp"

#include "../base/stl_add.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include <QMatrix4x4>


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

void DrapeSurface::CreateEngine()
{
  glClearColor(0.8, 0.8, 0.8, 1.0);

  GLFunctions::Init();
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glClearDepth(0.0);

  vector<UniformValue> uniforms;
  float model[16] =
  {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  };

  uniforms.push_back(UniformValue("modelView", model));
  float p[16] =
  {
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, -0.5, 0.0,
    0.0, 0.0, 0.0, 1.0
  };

  uniforms.push_back(UniformValue("projection", p));

  float color[4] = { 1.0, 0.0, 0.0, 1.0 };
  uniforms.push_back(UniformValue("color", color[0], color[1], color[2], color[3]));

  m_batcher->StartSession(bind(&DrapeSurface::FlushFullBucket, this, _1, _2));
  ListGenerator gen;
  gen.SetDepth(0.5);
  gen.SetProgram(gpu::SOLID_AREA_PROGRAM);
  gen.SetViewport(-1.0f, -1.0f, 2.0f, 2.0f);
  gen.SetUniforms(uniforms);
  gen.Generate(31, *m_batcher);
  m_batcher->EndSession();
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

void DrapeSurface::RenderBucket(const GLState & state, RefPointer<VertexArrayBuffer> bucket)
{
  RefPointer<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  ApplyState(state, program);
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
