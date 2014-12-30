#pragma once

#include "frontend_renderer.hpp"
#include "backend_renderer.hpp"
#include "threads_commutator.hpp"

#include "../drape/pointers.hpp"
#include "../drape/texture_manager.hpp"

#include "../geometry/screenbase.hpp"

namespace dp { class OGLContextFactory; }

namespace df
{

class MapDataProvider;
class Viewport;
class DrapeEngine
{
public:
  DrapeEngine(dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
              Viewport const & viewport,
              MapDataProvider const & model);
  ~DrapeEngine();

  void Resize(int w, int h);
  void UpdateCoverage(ScreenBase const & screen);

private:
  dp::MasterPointer<FrontendRenderer> m_frontend;
  dp::MasterPointer<BackendRenderer>  m_backend;
  dp::MasterPointer<ThreadsCommutator> m_threadCommutator;

  Viewport m_viewport;
};

} // namespace df
