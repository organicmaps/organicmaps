#pragma once

#include "qtoglcontextfactory.hpp"

#include "../drape/batcher.hpp"
#include "../drape/gpu_program_manager.hpp"
#include "../drape/uniform_values_storage.hpp"

#include <QtGui/QWindow>
#include <QtCore/QTimerEvent>

class DrapeSurface : public QWindow
{
  Q_OBJECT

public:
  DrapeSurface();
  ~DrapeSurface();

protected:
  void exposeEvent(QExposeEvent * e);
  void timerEvent(QTimerEvent * e);

private:
  void RefreshMVUniform(float angle);
  Q_SLOT void RefreshProjector(int);
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
  UniformValuesStorage m_generalUniforms;

  int m_timerID;

private:
  QtOGLContextFactory * m_contextFactory;
};
