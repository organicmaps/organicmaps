#pragma once

#include "drape/pointers.hpp"
#include "drape/oglcontextfactory.hpp"
#include "drape/batcher.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/uniform_values_storage.hpp"
#include "drape/texture_manager.hpp"

#include "drape_frontend/viewport.hpp"
#include "drape_frontend/map_data_provider.hpp"

#include "std/map.hpp"

#include <QObject>
#include <QEvent>

namespace df
{

class TestingEngine : public QObject
{
public:
  TestingEngine(dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
                Viewport const & viewport,
                double vs);
  ~TestingEngine();

  void Draw();
  void Resize(int w, int h);

protected:
  void timerEvent(QTimerEvent * e);

  int m_timerId;

private:
  void DrawImpl();
  void DrawRects();
  void ModelViewInit();
  void ProjectionInit();
  void OnFlushData(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> vao);
  void ClearScene();

private:
  dp::RefPointer<dp::OGLContextFactory> m_contextFactory;
  dp::MasterPointer<dp::Batcher> m_batcher;
  dp::MasterPointer<dp::GpuProgramManager> m_programManager;
  dp::MasterPointer<dp::TextureManager> m_textures;
  df::Viewport m_viewport;

  typedef map<dp::GLState, vector<dp::MasterPointer<dp::RenderBucket> > > TScene;
  TScene m_scene;
  ScreenBase m_modelView;
  float m_angle = 0.0;

  dp::UniformValuesStorage m_generalUniforms;

  vector<m2::RectD> m_boundRects;
  vector<dp::OverlayHandle::Rects> m_rects;
};

} // namespace df
