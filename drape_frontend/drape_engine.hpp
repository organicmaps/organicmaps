#pragma once

#include "frontend_renderer.hpp"
#include "backend_renderer.hpp"
#include "threads_commutator.hpp"

#include "../drape/pointers.hpp"
#include "../drape/oglcontextfactory.hpp"

namespace df
{
  class Viewport;
  class DrapeEngine
  {
  public:
    DrapeEngine(RefPointer<OGLContextFactory> oglcontextfactory, double vs, Viewport const & viewport);
    ~DrapeEngine();

    void OnSizeChanged(int x0, int y0, int w, int h);
    void SetAngle(float radians);

  private:
    MasterPointer<FrontendRenderer> m_frontend;
    MasterPointer<BackendRenderer>  m_backend;

    MasterPointer<ThreadsCommutator> m_threadCommutator;
  };
}
