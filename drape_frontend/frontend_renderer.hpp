#pragma once

#include "base/thread.hpp"

#ifdef DRAW_INFO
  #include "base/timer.hpp"
  #include "std/vector.hpp"
  #include "std/numeric.hpp"
#endif

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/threads_commutator.hpp"
#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/tile_tree.hpp"
#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/render_group.hpp"
#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/navigator.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "drape_gui/layer_render.hpp"

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

class FrontendRenderer : public BaseRenderer
                       , public MyPositionController::Listener
                       , public UserEventStream::Listener
{
public:
  using TModelViewChanged = function<void (ScreenBase const & screen)>;
  using TIsCountryLoaded = TIsCountryLoaded;

  struct Params : BaseRenderer::Params
  {
    Params(ref_ptr<ThreadsCommutator> commutator,
           ref_ptr<dp::OGLContextFactory> factory,
           ref_ptr<dp::TextureManager> texMng,
           Viewport viewport,
           TModelViewChanged const & modelViewChangedFn,
           TIsCountryLoaded const & isCountryLoaded,
           location::TMyPositionModeChanged myPositionModeCallback,
           location::EMyPositionMode initMode)
      : BaseRenderer::Params(commutator, factory, texMng)
      , m_viewport(viewport)
      , m_modelViewChangedFn(modelViewChangedFn)
      , m_isCountryLoadedFn(isCountryLoaded)
      , m_myPositionModeCallback(myPositionModeCallback)
      , m_initMyPositionMode(initMode)
    {}

    Viewport m_viewport;
    TModelViewChanged m_modelViewChangedFn;
    TIsCountryLoaded m_isCountryLoadedFn;
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
  void RefreshProjection();
  void RefreshModelView(ScreenBase const & screen);
  ScreenBase const & UpdateScene(bool & modelViewChanged);

  void EmitModelViewChanged(ScreenBase const & modelView) const;

  void ResolveTileKeys(ScreenBase const & screen, TTilesCollection & tiles);
  int GetCurrentZoomLevel() const;
  void ResolveZoomLevel(ScreenBase const & screen);

  void OnTap(m2::PointD const & pt, bool isLong) override;
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

private:
  drape_ptr<dp::GpuProgramManager> m_gpuProgramManager;

  vector<drape_ptr<RenderGroup>> m_renderGroups;
  vector<drape_ptr<RenderGroup>> m_deferredRenderGroups;
  vector<drape_ptr<UserMarkRenderGroup>> m_userMarkRenderGroups;
  set<TileKey> m_userMarkVisibility;

  drape_ptr<gui::LayerRenderer> m_guiRenderer;
  drape_ptr<MyPositionController> m_myPositionController;

  drape_ptr<dp::OverlayTree> m_overlayTree;
  bool m_overlayTreeIsUpdating;

  dp::UniformValuesStorage m_generalUniforms;

  Viewport m_viewport;
  UserEventStream m_userEventStream;
  TModelViewChanged m_modelViewChangedFn;

  unique_ptr<TileTree> m_tileTree;
  int m_currentZoomLevel = -1;
};

} // namespace df
