#pragma once

#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/screenbase.hpp"

#include "base/strings_bundle.hpp"

namespace dp { class OGLContextFactory; }
namespace gui { class StorageAccessor; }

namespace df
{

class UserMarksProvider;
class MapDataProvider;
class Viewport;

class DrapeEngine
{
public:
  struct Params
  {
    Params(ref_ptr<dp::OGLContextFactory> factory,
           ref_ptr<StringsBundle> stringBundle,
           ref_ptr<gui::StorageAccessor> storageAccessor,
           Viewport const & viewport,
           MapDataProvider const & model,
           double vs)
      : m_factory(factory)
      , m_stringsBundle(stringBundle)
      , m_storageAccessor(storageAccessor)
      , m_viewport(viewport)
      , m_model(model)
      , m_vs(vs)
    {
    }

    ref_ptr<dp::OGLContextFactory> m_factory;
    ref_ptr<StringsBundle> m_stringsBundle;
    ref_ptr<gui::StorageAccessor> m_storageAccessor;
    Viewport m_viewport;
    MapDataProvider m_model;
    double m_vs;
  };

  DrapeEngine(Params const & params);
  ~DrapeEngine();

  void Resize(int w, int h);
  void UpdateCoverage(ScreenBase const & screen);

  void ClearUserMarksLayer(TileKey const & tileKey);
  void ChangeVisibilityUserMarksLayer(TileKey const & tileKey, bool isVisible);
  void UpdateUserMarksLayer(TileKey const & tileKey, UserMarksProvider * provider);

  void SetRenderingEnabled(bool const isEnabled);

private:
  drape_ptr<FrontendRenderer> m_frontend;
  drape_ptr<BackendRenderer> m_backend;
  drape_ptr<ThreadsCommutator> m_threadCommutator;
  drape_ptr<dp::TextureManager> m_textureManager;

  Viewport m_viewport;
};

} // namespace df
