#pragma once

#include "../drape/pointers.hpp"
#include "../drape/oglcontextfactory.hpp"
#include "../drape/batcher.hpp"
#include "../drape/gpu_program_manager.hpp"
#include "../drape/uniform_values_storage.hpp"
#include "../drape/texture_manager.hpp"

#include "../drape_frontend/viewport.hpp"

#include "../std/map.hpp"

#include <QObject>
#include <QEvent>

namespace df
{

class TestingEngine : public QObject
{
public:
  TestingEngine(RefPointer<OGLContextFactory> oglcontextfactory, double vs, df::Viewport const & viewport);
  ~TestingEngine();

  void Draw();
  void Resize(int w, int h);
  void DragStarted(m2::PointF const & p);
  void Drag(m2::PointF const & p);
  void DragEnded(m2::PointF const & p);
  void Scale(m2::PointF const & p, double factor);

protected:
  void timerEvent(QTimerEvent * e);

  int m_timerId;

private:
  void DrawImpl();
  void ModelViewInit();
  void ProjectionInit();
  void OnFlushData(GLState const & state, TransferPointer<RenderBucket> vao);
  void ClearScene();

private:
  RefPointer<OGLContextFactory> m_contextFactory;
  MasterPointer<Batcher> m_batcher;
  MasterPointer<GpuProgramManager> m_programManager;
  MasterPointer<TextureManager> m_textures;
  df::Viewport m_viewport;

  typedef map<GLState, vector<MasterPointer<RenderBucket> > > TScene;
  TScene m_scene;

  UniformValuesStorage m_generalUniforms;
};

} // namespace df
