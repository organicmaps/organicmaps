#pragma once

#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "drape_gui/country_status_helper.hpp"

#include "geometry/screenbase.hpp"

#include "base/strings_bundle.hpp"

#include "std/map.hpp"
#include "std/mutex.hpp"

namespace dp { class OGLContextFactory; }

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
           Viewport const & viewport,
           MapDataProvider const & model,
           double vs)
      : m_factory(factory)
      , m_stringsBundle(stringBundle)
      , m_viewport(viewport)
      , m_model(model)
      , m_vs(vs)
    {
    }

    ref_ptr<dp::OGLContextFactory> m_factory;
    ref_ptr<StringsBundle> m_stringsBundle;
    Viewport m_viewport;
    MapDataProvider m_model;
    double m_vs;
  };

  DrapeEngine(Params const & params);
  ~DrapeEngine();

  void Resize(int w, int h);

  void AddTouchEvent(TouchEvent const & event);
  void Scale(double  factor, m2::PointD const & pxPoint, bool isAnim);

  /// if zoom == -1, then current zoom will not change
  void SetModelViewCenter(m2::PointD const & centerPt, int zoom, bool isAnim);
  void SetModelViewRect(m2::RectD const & rect, bool applyRotation, int zoom, bool isAnim);
  void SetModelViewAnyRect(m2::AnyRectD const & rect, bool isAnim);

  using TModelViewListenerFn = FrontendRenderer::TModelViewChanged;
  int AddModelViewListener(TModelViewListenerFn const & listener);
  void RemoveModeViewListener(int slotID);

  void ClearUserMarksLayer(TileKey const & tileKey);
  void ChangeVisibilityUserMarksLayer(TileKey const & tileKey, bool isVisible);
  void UpdateUserMarksLayer(TileKey const & tileKey, UserMarksProvider * provider);

  void SetRenderingEnabled(bool const isEnabled);

  void SetStorageInfo(gui::StorageInfo const & info, bool isCurrentCountry);

private:
  void AddUserEvent(UserEvent const & e);
  void ModelViewChanged(ScreenBase const & screen);
  void ModelViewChangedGuiThread(ScreenBase const & screen);

private:
  drape_ptr<FrontendRenderer> m_frontend;
  drape_ptr<BackendRenderer> m_backend;
  drape_ptr<ThreadsCommutator> m_threadCommutator;
  drape_ptr<dp::TextureManager> m_textureManager;

  Viewport m_viewport;

  using TListenerMap = map<int, TModelViewListenerFn>;
  TListenerMap m_listeners;
};

} // namespace df
