#pragma once

#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/screenbase.hpp"

namespace dp { class OGLContextFactory; }

namespace df
{

class UserMarksProvider;
class MapDataProvider;
class Viewport;

class DrapeEngine
{
public:
  DrapeEngine(dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
              Viewport const & viewport,
              MapDataProvider const & model,
              double vs);
  ~DrapeEngine();

  void Resize(int w, int h);
  void UpdateCoverage(ScreenBase const & screen);

  void ClearUserMarksLayer(TileKey const & tileKey);
  void ChangeVisibilityUserMarksLayer(TileKey const & tileKey, bool isVisible);
  void UpdateUserMarksLayer(TileKey const & tileKey, UserMarksProvider * provider);

private:
  dp::MasterPointer<FrontendRenderer> m_frontend;
  dp::MasterPointer<BackendRenderer>  m_backend;
  dp::MasterPointer<ThreadsCommutator> m_threadCommutator;

  Viewport m_viewport;
};

} // namespace df
