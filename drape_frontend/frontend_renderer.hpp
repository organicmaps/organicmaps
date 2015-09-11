#pragma once

#include "base/thread.hpp"

#ifdef DRAW_INFO
  #include "base/timer.hpp"
  #include "std/vector.hpp"
  #include "std/numeric.hpp"
#endif

#include "drape_frontend/gui/layer_render.hpp"

#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/navigator.hpp"
#include "drape_frontend/render_group.hpp"
#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/threads_commutator.hpp"
#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/tile_tree.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "drape/pointers.hpp"
#include "drape/glstate.hpp"
#include "drape/vertex_array_buffer.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/overlay_tree.hpp"
#include "drape/uniform_values_storage.hpp"

#include "platform/location.hpp"

#include "geometry/screenbase.hpp"

#include "std/function.hpp"
#include "std/map.hpp"

namespace dp
{
  class RenderBucket;
  class OverlayTree;
}

namespace df
{

class SelectionShape;

struct TapInfo
{
  m2::PointD const m_pixelPoint;
  bool m_isLong;

  bool m_isMyPositionTapped;
  FeatureID m_featureTapped;
};

class FrontendRenderer : public BaseRenderer
                       , public MyPositionController::Listener
                       , public UserEventStream::Listener
{
public:
  using TModelViewChanged = function<void (ScreenBase const & screen)>;
  using TIsCountryLoaded = TIsCountryLoaded;
  using TTapEventInfoFn = function<void (m2::PointD const & pxPoint, bool isLong, bool isMyPosition, FeatureID const & id)>;
  using TUserPositionChangedFn = function<void (m2::PointD const & pt)>;

  struct Params : BaseRenderer::Params
  {
    Params(ref_ptr<ThreadsCommutator> commutator,
           ref_ptr<dp::OGLContextFactory> factory,
           ref_ptr<dp::TextureManager> texMng,
           Viewport viewport,
           TModelViewChanged const & modelViewChangedFn,
           TIsCountryLoaded const & isCountryLoaded,
           TTapEventInfoFn const & tapEventFn,
           TUserPositionChangedFn const & positionChangedFn,
           location::TMyPositionModeChanged myPositionModeCallback,
           location::EMyPositionMode initMode)
      : BaseRenderer::Params(commutator, factory, texMng)
      , m_viewport(viewport)
      , m_modelViewChangedFn(modelViewChangedFn)
      , m_isCountryLoadedFn(isCountryLoaded)
      , m_tapEventFn(tapEventFn)
      , m_positionChangedFn(positionChangedFn)
      , m_myPositionModeCallback(myPositionModeCallback)
      , m_initMyPositionMode(initMode)
    {}

    Viewport m_viewport;
    TModelViewChanged m_modelViewChangedFn;
    TIsCountryLoaded m_isCountryLoadedFn;
    TTapEventInfoFn m_tapEventFn;
    TUserPositionChangedFn m_positionChangedFn;
    location::TMyPositionModeChanged m_myPositionModeCallback;
    location::EMyPositionMode m_initMyPositionMode;
  };

  FrontendRenderer(Params const & params);
  ~FrontendRenderer() override;

#ifdef DRAW_INFO
  double m_tpf;
  double m_fps;

  my::Timer m_timer;
  double m_frameStartTime;
  vector<double> m_tpfs;
  int m_drawedFrames;

  void BeforeDrawFrame();
  void AfterDrawFrame();
#endif

  void AddUserEvent(UserEvent const & event);

  /// MyPositionController::Listener
  void PositionChanged(m2::PointD const & position) override;
  void ChangeModelView(m2::PointD const & center) override;
  void ChangeModelView(double azimuth) override;
  void ChangeModelView(m2::RectD const & rect) override;
  void ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero) override;

protected:
  virtual void AcceptMessage(ref_ptr<Message> message);
  unique_ptr<threads::IRoutine> CreateRoutine() override;

private:
  void OnResize(ScreenBase const & screen);
  void RenderScene(ScreenBase const & modelView);
  void RenderSingleGroup(ScreenBase const & modelView, ref_ptr<BaseRenderGroup> group);
  void RefreshProjection();
  void RefreshModelView(ScreenBase const & screen);
  void RefreshBgColor();
  ScreenBase const & UpdateScene(bool & modelViewChanged);

  void EmitModelViewChanged(ScreenBase const & modelView) const;

  void ResolveTileKeys(ScreenBase const & screen, TTilesCollection & tiles);
  void ResolveTileKeys(m2::RectD const & rect, TTilesCollection & tiles);
  int GetCurrentZoomLevel() const;
  void ResolveZoomLevel(ScreenBase const & screen);

  void OnTap(m2::PointD const & pt, bool isLong) override;
  void OnDoubleTap(m2::PointD const & pt) override;
  bool OnSingleTouchFiltrate(m2::PointD const & pt, TouchEvent::ETouchType type) override;
  void OnDragStarted() override;
  void OnDragEnded(m2::PointD const & distance) override;

  void OnScaleStarted() override;
  void OnRotated() override;
  void CorrectScalePoint(m2::PointD & pt) const override;
  void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const override;
  void OnScaleEnded() override;

  class Routine : public threads::IRoutine
  {
   public:
    Routine(FrontendRenderer & renderer);

    // threads::IRoutine overrides:
    void Do() override;

   private:
    FrontendRenderer & m_renderer;
  };

  void ReleaseResources();

  void BeginUpdateOverlayTree(ScreenBase const & modelView);
  void UpdateOverlayTree(ScreenBase const & modelView, drape_ptr<RenderGroup> & renderGroup);
  void EndUpdateOverlayTree();

  void AddToRenderGroup(vector<drape_ptr<RenderGroup>> & groups,
                        dp::GLState const & state,
                        drape_ptr<dp::RenderBucket> && renderBucket,
                        TileKey const & newTile);
  void OnAddRenderGroup(TileKey const & tileKey, dp::GLState const & state,
                        drape_ptr<dp::RenderBucket> && renderBucket);
  void OnDeferRenderGroup(TileKey const & tileKey, dp::GLState const & state,
                          drape_ptr<dp::RenderBucket> && renderBucket);

  void OnActivateTile(TileKey const & tileKey);
  void OnRemoveTile(TileKey const & tileKey);

  void OnCompassTapped();

  FeatureID GetVisiblePOI(m2::PointD const & pixelPoint) const;
  FeatureID GetVisiblePOI(m2::RectD const & pixelRect) const;

private:
  drape_ptr<dp::GpuProgramManager> m_gpuProgramManager;

  vector<drape_ptr<RenderGroup>> m_renderGroups;
  vector<drape_ptr<RenderGroup>> m_deferredRenderGroups;
  vector<drape_ptr<UserMarkRenderGroup>> m_userMarkRenderGroups;
  set<TileKey> m_userMarkVisibility;

  drape_ptr<gui::LayerRenderer> m_guiRenderer;
  drape_ptr<MyPositionController> m_myPositionController;
  drape_ptr<SelectionShape> m_selectionShape;
  drape_ptr<RouteRenderer> m_routeRenderer;

  drape_ptr<dp::OverlayTree> m_overlayTree;

  dp::UniformValuesStorage m_generalUniforms;

  Viewport m_viewport;
  UserEventStream m_userEventStream;
  TModelViewChanged m_modelViewChangedFn;
  TTapEventInfoFn m_tapEventInfoFn;
  TUserPositionChangedFn m_userPositionChangedFn;

  unique_ptr<TileTree> m_tileTree;
  int m_currentZoomLevel = -1;
};

} // namespace df
