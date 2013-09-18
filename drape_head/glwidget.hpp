#pragma once

#include <QtOpenGL/QGLWidget>

#include "../drape/batcher.hpp"
#include "../drape/gpu_program_manager.hpp"

class GLWidget : public QGLWidget, public IBatchFlush
{
public:
  GLWidget();

  virtual void FlushFullBucket(const GLState & state, OwnedPointer<VertexArrayBuffer> bucket);
  virtual void UseIncompleteBucket(const GLState & state, ReferencePoiner<VertexArrayBuffer> bucket);

protected:
  void initializeGL();
  void paintGL();
  void resizeGL(int w, int h);

private:
  void renderBucket(const GLState & state, ReferencePoiner<VertexArrayBuffer> bucket);

private:
  typedef map<GLState, vector<OwnedPointer<VertexArrayBuffer> > > frames_t;
  frames_t m_frames;

private:
  Batcher * m_batcher;
  GpuProgramManager * m_programManager;
};
