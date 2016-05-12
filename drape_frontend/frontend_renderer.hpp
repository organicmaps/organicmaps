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
#include "drape_frontend/gps_track_renderer.hpp"
#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/navigator.hpp"
#include "drape_frontend/render_group.hpp"
#include "drape_frontend/requested_tiles.hpp"
#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/threads_commutator.hpp"
#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "drape/pointers.hpp"
#include "drape/glstate.hpp"
#include "drape/vertex_array_buffer.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/overlay_tree.hpp"
#include "drape/uniform_values_storage.hpp"

#include "platform/location.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/triangle2d.hpp"

#include "std/function.hpp"
#include "std/map.hpp"
#include "std/array.hpp"

namespace dp
{
  class RenderBucket;
  class OverlayTree;
}

namespace df
{

class SelectionShape;
class Framebuffer;
class TransparentLayer;
class SelectObjectMessage;

struct TapInfo
{
  m2::PointD const m_pixelPoint;
  bool const m_isLong;
  bool const m_isMyPositionTapped;
  FeatureID const m_featureTapped;
};

class FrontendRenderer : public BaseRenderer
                       , public MyPositionController::Listener
                       , public UserEventStream::Listener
{
public:
  using TModelViewChanged = function<void (ScreenBase const & screen)>;
  using TTapEventInfoFn = function<void (TapInfo const &)>;
  using TUserPositionChangedFn = function<void (m2::PointD const & pt)>;

  struct Params : BaseRenderer::Params
  {
    Params(ref_ptr<ThreadsCommutator> commutator,
           ref_ptr<dp::OGLContextFactory> factory,
           ref_ptr<dp::TextureManager> texMng,
           Viewport viewport,
           TModelViewChanged const & modelViewChangedFn,
           TTapEventInfoFn const & tapEventFn,
           TUserPositionChangedFn const & positionChangedFn,
           location::TMyPositionModeChanged myPositionModeCallback,
           location::EMyPositionMode initMode,
           ref_ptr<RequestedTiles> requestedTiles,
           double timeInBackground,
           bool allow3dBuildings,
           bool blockTapEvents,
           bool firstLaunch,
           bool isRoutingActive)
      : BaseRenderer::Params(commutator, factory, texMng)
      , m_viewport(viewport)
      , m_modelViewChangedFn(modelViewChangedFn)
      , m_tapEventFn(tapEventFn)
      , m_positionChangedFn(positionChangedFn)
      , m_myPositionModeCallback(myPositionModeCallback)
      , m_initMyPositionMode(initMode)
      , m_requestedTiles(requestedTiles)
      , m_timeInBackground(timeInBackground)
      , m_allow3dBuildings(allow3dBuildings)
      , m_blockTapEvents(blockTapEvents)
      , m_firstLaunch(firstLaunch)
      , m_isRoutingActive(isRoutingActive)
    {}

    Viewport m_viewport;
    TModelViewChanged m_modelViewChangedFn;
    TTapEventInfoFn m_tapEventFn;
    TUserPositionChangedFn m_positionChangedFn;
    location::TMyPositionModeChanged m_myPositionModeCallback;
    location::EMyPositionMode m_initMyPositionMode;
    ref_ptr<RequestedTiles> m_requestedTiles;
    double m_timeInBackground;
    bool m_allow3dBuildings;
    bool m_blockTapEvents;
    bool m_firstLaunch;
    bool m_isRoutingActive;
  };

  FrontendRenderer(Params const & params);
  ~FrontendRenderer() override;

  void Teardown();

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
  void ChangeModelView(m2::PointD const & center, int zoomLevel) override;
  void ChangeModelView(double azimuth) override;
  void ChangeModelView(m2::RectD const & rect) override;
  void ChangeModelView(m2::PointD const & userPos, double azimuth,
                       m2::PointD const & pxZero, int preferredZoomLevel) override;

protected:
  void AcceptMessage(ref_ptr<Message> message) override;
  unique_ptr<threads::IRoutine> CreateRoutine() override;

private:
  void OnResize(ScreenBase const & screen);
  void RenderScene(ScreenBase const & modelView);
  void PrepareBucket(dp::GLState const & state, drape_ptr<dp::RenderBucket> & bucket);
  void MergeBuckets();
  void RenderSingleGroup(ScreenBase const & modelView, ref_ptr<BaseRenderGroup> group);
  void RefreshProjection(ScreenBase const & screen);
  void RefreshModelView(ScreenBase const & screen);
  void RefreshPivotTransform(ScreenBase const & screen);
  void RefreshBgColor();

  //////
  /// Render part of scene
  void Render2dLayer(ScreenBase const & modelView);
  void Render3dLayer(ScreenBase const & modelView);
  void RenderOverlayLayer(ScreenBase const & modelView);
  //////
  ScreenBase const & ProcessEvents(bool & modelViewChanged, bool & viewportChanged);
  void PrepareScene(ScreenBase const & modelView);
  void UpdateScene(ScreenBase const & modelView);
  void BuildOverlayTree(ScreenBase const & modelView);

  void EmitModelViewChanged(ScreenBase const & modelView) const;

  TTilesCollection ResolveTileKeys(ScreenBase const & screen);
  void ResolveZoomLevel(ScreenBase const & screen);
  void UpdateDisplacementEnabled();
  void CheckPerspectiveMinScale();
  void CheckIsometryMinScale(ScreenBase const & screen);

  void DisablePerspective();

  void OnTap(m2::PointD const & pt, bool isLong) override;
  void OnForceTap(m2::PointD const & pt) override;
  void OnDoubleTap(m2::PointD const & pt) override;
  void OnTwoFingersTap() override;
  bool OnSingleTouchFiltrate(m2::PointD const & pt, TouchEvent::ETouchType type) override;
  void OnDragStarted() override;
  void OnDragEnded(m2::PointD const & distance) override;

  void OnScaleStarted() override;
  void OnRotated() override;
  void CorrectScalePoint(m2::PointD & pt) const override;
  void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const override;
  void CorrectGlobalScalePoint(m2::PointD & pt) const override;
  void OnScaleEnded() override;
  void OnAnimationStarted(ref_ptr<Animation> anim) override;
  void OnPerspectiveSwitchRejected() override;
  void OnTouchMapAction() override;

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

  void AddToRenderGroup(dp::GLState const & state,
                        drape_ptr<dp::RenderBucket> && renderBucket,
                        TileKey const & newTile);

  using TRenderGroupRemovePredicate = function<bool(drape_ptr<RenderGroup> const &)>;
  void RemoveRenderGroupsLater(TRenderGroupRemovePredicate const & predicate);

  void FollowRoute(int preferredZoomLevel, int preferredZoomLevelIn3d,
                   double rotationAngle, double angleFOV);
  void InvalidateRect(m2::RectD const & gRect);
  bool CheckTileGenerations(TileKey const & tileKey);
  void UpdateCanBeDeletedStatus();

  void OnCompassTapped();

  FeatureID GetVisiblePOI(m2::PointD const & pixelPoint);
  FeatureID GetVisiblePOI(m2::RectD const & pixelRect);

  bool IsPerspective() const;

  void PrepareGpsTrackPoints(size_t pointsCount);

  void PullToBoundArea(bool randomPlace, bool applyZoom);

  void ProcessSelection(ref_ptr<SelectObjectMessage> msg);

  drape_ptr<dp::GpuProgramManager> m_gpuProgramManager;

  struct RenderLayer
  {
    enum RenderLayerID
    {
      Geometry2dID,
      OverlayID,
      Geometry3dID,
      LayerCountID
    };

    static RenderLayerID GetLayerID(dp::GLState const & renderGroup);

    vector<drape_ptr<RenderGroup>> m_renderGroups;
    bool m_isDirty = false;

    void Sort(ref_ptr<dp::OverlayTree> overlayTree);
  };

  array<RenderLayer, RenderLayer::LayerCountID> m_layers;
  vector<drape_ptr<UserMarkRenderGroup>> m_userMarkRenderGroups;
  set<TileKey> m_userMarkVisibility;

  drape_ptr<gui::LayerRenderer> m_guiRenderer;
  drape_ptr<MyPositionController> m_myPositionController;
  drape_ptr<SelectionShape> m_selectionShape;
  drape_ptr<RouteRenderer> m_routeRenderer;
  drape_ptr<Framebuffer> m_framebuffer;
  drape_ptr<TransparentLayer> m_transparentLayer;
  drape_ptr<GpsTrackRenderer> m_gpsTrackRenderer;

  drape_ptr<dp::OverlayTree> m_overlayTree;

  dp::UniformValuesStorage m_generalUniforms;

  bool m_enablePerspectiveInNavigation;
  bool m_enable3dBuildings;
  bool m_isIsometry;

  bool m_blockTapEvents;

  bool m_choosePositionMode;

  Viewport m_viewport;
  UserEventStream m_userEventStream;
  TModelViewChanged m_modelViewChangedFn;
  TTapEventInfoFn m_tapEventInfoFn;
  TUserPositionChangedFn m_userPositionChangedFn;

  ScreenBase m_lastReadedModelView;
  TTilesCollection m_notFinishedTiles;

  int m_currentZoomLevel = -1;

  bool m_perspectiveDiscarded = false;
  
  ref_ptr<RequestedTiles> m_requestedTiles;
  uint64_t m_maxGeneration;
  int m_mergeBucketsCounter = 0;

  struct FollowRouteData
  {
    FollowRouteData(int preferredZoomLevel,
                    int preferredZoomLevelIn3d,
                    double rotationAngle,
                    double angleFOV)
      : m_preferredZoomLevel(preferredZoomLevel)
      , m_preferredZoomLevelIn3d(preferredZoomLevelIn3d)
      , m_rotationAngle(rotationAngle)
      , m_angleFOV(angleFOV)
    {}

    int m_preferredZoomLevel;
    int m_preferredZoomLevelIn3d;
    double m_rotationAngle;
    double m_angleFOV;
  };

  unique_ptr<FollowRouteData> m_pendingFollowRoute;

  vector<m2::TriangleD> m_dragBoundArea;

  drape_ptr<SelectObjectMessage> m_selectObjectMessage;

#ifdef DEBUG
  bool m_isTeardowned;
#endif
};

} // namespace df
