#pragma once

#include "../drape/pointers.hpp"
#include "../drape/oglcontextfactory.hpp"
#include "../drape/batcher.hpp"
#include "../drape/gpu_program_manager.hpp"
#include "../drape/uniform_values_storage.hpp"

#include "../drape_frontend/viewport.hpp"

#include <map>

namespace df
{
  class TestingEngine
  {
  public:
    TestingEngine(RefPointer<OGLContextFactory> oglcontextfactory, double vs, df::Viewport const & viewport);
    ~TestingEngine();

    void Draw();
    void OnSizeChanged(int x0, int y0, int w, int h);
    void SetAngle(float radians);

  private:
    void DrawImpl();
    void ModelViewInit();
    void ProjectionInit();
    void OnFlushData(const GLState & state, TransferPointer<VertexArrayBuffer> vao);
    void ClearScene();

  private:
    RefPointer<OGLContextFactory> m_contextFactory;
    MasterPointer<Batcher> m_batcher;
    MasterPointer<GpuProgramManager> m_programManager;
    df::Viewport m_viewport;

    typedef map<GLState, MasterPointer<VertexArrayBuffer> > scene_t;
    scene_t m_scene;

    UniformValuesStorage m_generalUniforms;
  };
}
