#pragma once

#include "frontend_renderer.hpp"
#include "backend_renderer.hpp"
#include "threads_commutator.hpp"

#include "../drape/pointers.hpp"
#include "../drape/texture_manager.hpp"

#include "../map/navigator.hpp"

class OGLContextFactory;

namespace df
{
  class Viewport;
  class DrapeEngine
  {
  public:
    DrapeEngine(RefPointer<OGLContextFactory> oglcontextfactory, double vs, Viewport const & viewport);
    ~DrapeEngine();

    void Resize(int w, int h);
    void DragStarted(m2::PointF const & p);
    void Drag(m2::PointF const & p);
    void DragEnded(m2::PointF const & p);
    void Scale(m2::PointF const & p, double factor);

  private:
    void UpdateCoverage();

  private:
    MasterPointer<FrontendRenderer> m_frontend;
    MasterPointer<BackendRenderer>  m_backend;

    MasterPointer<TextureManager> m_textures;
    MasterPointer<ThreadsCommutator> m_threadCommutator;

    ScalesProcessor m_scales;
    Viewport m_viewport;
    Navigator m_navigator;
  };
}
