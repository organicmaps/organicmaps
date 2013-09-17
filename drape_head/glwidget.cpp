#include "glwidget.hpp"

GLWidget::GLWidget()
{
  m_batcher = new Batcher(WeakPointer<IBatchFlush>(this));
  m_programManager = new GpuProgramManager();
}

void GLWidget::FlushFullBucket(const GLState & state, StrongPointer<VertexArrayBuffer> bucket)
{
  WeakPointer<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  bucket->BuildVertexArray(program);
  m_frames[state].push_back(bucket);
}

void GLWidget::UseIncompleteBucket(const GLState &state, WeakPointer<VertexArrayBuffer> bucket)
{
  WeakPointer<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  bucket->BuildVertexArray(program);
  renderBucket(state, bucket);
}

void GLWidget::initializeGL()
{
  glClearColor(0.8, 0.8, 0.8, 1.0);

  GLFunctions::Init();
  TextureBinding binding("", false, 0, WeakPointer<Texture>(NULL));
  GLState s(1, 0, binding);
  vector<UniformValue> & uniforms = s.GetUniformValues();
  uniforms.push_back(UniformValue("depth", 0.0f));
  uniforms.push_back(UniformValue("color", 0.0f, 0.65f, 0.35f, 1.0f));

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

  StrongPointer<AttributeProvider> provider(new AttributeProvider(1, 4));
  BindingInfo info(1);
  BindingDecl & decl =  info.GetBindingDecl(0);
  decl.m_attributeName = "position";
  decl.m_componentCount = 2;
  decl.m_componentType = GLConst::GLFloatType;
  decl.m_offset = 0;
  decl.m_stride = 0;

  float data[2 * 4]=
  {
    -1.0, -1.0,
     1.0, -1.0,
    -1.0,  1.0,
     1.0,  1.0
  };

  provider->InitStream(0, info, WeakPointer<void>(data));
  m_batcher->InsertTriangleStrip(s, provider.GetWeakPointer());

  provider.Destroy();
}

void GLWidget::paintGL()
{
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

void GLWidget::renderBucket(const GLState & state, WeakPointer<VertexArrayBuffer> bucket)
{
  WeakPointer<GpuProgram> program = m_programManager->GetProgram(state.GetProgramIndex());
  ApplyState(state, program);
  bucket->Render();
}

void GLWidget::resizeGL(int w, int h)
{
  glViewport(0, 0, w, h);
}
