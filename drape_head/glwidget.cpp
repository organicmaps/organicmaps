#include "glwidget.hpp"
#include "../drape/utils/list_generator.hpp"

GLWidget::GLWidget()
{
  m_batcher = new Batcher(ReferencePoiner<IBatchFlush>(this));
  m_programManager = new GpuProgramManager();
}

void GLWidget::FlushFullBucket(const GLState & state, OwnedPointer<VertexArrayBuffer> bucket)
{
  ReferencePoiner<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  bucket->BuildVertexArray(program);
  m_frames[state].push_back(bucket);
}

void GLWidget::UseIncompleteBucket(const GLState &state, ReferencePoiner<VertexArrayBuffer> bucket)
{
  ReferencePoiner<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  bucket->BuildVertexArray(program);
  renderBucket(state, bucket);
}

void GLWidget::initializeGL()
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
    -1.0, 0.0,  0.0, 0.0,
     0.0, 1.0,  0.0, 0.0,
     0.0, 0.0, -1.0, 0.0,
     0.0, 0.0, -1.0, 1.0
  };

  uniforms.push_back(UniformValue("modelViewMatrix", model));
  float p[16] =
  {
    0.5,  0.0, 0.0, 0.0,
    0.0, -0.5, 0.0, 0.0,
    0.0,  0.0, 1.0, 0.0,
    0.0,  0.0, 0.0, 1.0
  };

  uniforms.push_back(UniformValue("projectionMatrix", p));

  ListGenerator gen;
  gen.SetDepth(0.5);
  gen.SetProgram(1);
  gen.SetViewport(-1.0f, -1.0f, 2.0f, 2.0f);
  gen.SetUniforms(uniforms);
  gen.Generate(3000, *m_batcher);
}

void GLWidget::paintGL()
{
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
        renderBucket(it->first, it->second[i].GetWeakPointer());
      }
    }
  }
  else
    m_batcher->RequestIncompleteBuckets();
}

void GLWidget::renderBucket(const GLState & state, ReferencePoiner<VertexArrayBuffer> bucket)
{
  ReferencePoiner<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  ApplyState(state, program);
  bucket->Render();
}

void GLWidget::resizeGL(int w, int h)
{
  glViewport(0, 0, w, h);
}
