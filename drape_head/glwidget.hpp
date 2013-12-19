#pragma once

#include <QtOpenGL/QGLWidget>

#include "../drape/batcher.hpp"
#include "../drape/gpu_program_manager.hpp"

class GLWidget : public QGLWidget
{
public:
  GLWidget();
  ~GLWidget();

  virtual void FlushFullBucket(const GLState & state, TransferPointer<VertexArrayBuffer> bucket);

protected:
  void initializeGL();
  void paintGL();
  void resizeGL(int w, int h);

private:
  void renderBucket(const GLState & state, RefPointer<VertexArrayBuffer> bucket);

private:
  typedef map<GLState, vector<MasterPointer<VertexArrayBuffer> > > frames_t;
  frames_t m_frames;

private:
  Batcher * m_batcher;
  GpuProgramManager * m_programManager;
};
