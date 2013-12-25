#pragma once

#include "qtoglcontextfactory.hpp"

#include "../drape/batcher.hpp"
#include "../drape/gpu_program_manager.hpp"

#include <QtGui/QWindow>

class DrapeSurface : public QWindow
{
public:
  DrapeSurface();
  ~DrapeSurface();

protected:
  void exposeEvent(QExposeEvent * e);

private:
  void CreateEngine();
  void Render();
  void RenderBucket(const GLState & state, RefPointer<VertexArrayBuffer> bucket);
  void FlushFullBucket(const GLState & state, TransferPointer<VertexArrayBuffer> bucket);

private:
  typedef map<GLState, vector<MasterPointer<VertexArrayBuffer> > > frames_t;
  frames_t m_frames;

private:
  Batcher * m_batcher;
  GpuProgramManager * m_programManager;

private:
  QtOGLContextFactory * m_contextFactory;
};
