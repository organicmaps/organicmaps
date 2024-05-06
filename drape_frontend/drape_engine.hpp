#pragma once

#include "traffic/traffic_info.hpp"

#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/custom_features_context.hpp"
#include "drape_frontend/drape_engine_params.hpp"
#include "drape_frontend/drape_hints.hpp"
#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/overlays_tracker.hpp"
#include "drape_frontend/postprocess_renderer.hpp"
#include "drape_frontend/route_shape.hpp"
#include "drape_frontend/scenario_manager.hpp"
#include "drape_frontend/selection_shape.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"
#include "drape/viewport.hpp"

#include "transit/transit_display_info.hpp"

#include "platform/location.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/triangle2d.hpp"

#include "base/strings_bundle.hpp"

#include <atomic>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace dp
{
class GlyphGenerator;
class GraphicsContextFactory;
}  // namespace dp

namespace df
{
class UserMarksProvider;
class MapDataProvider;

class DrapeEngine
{
public:
  struct Params
  {
    Params(dp::ApiVersion apiVersion, ref_ptr<dp::GraphicsContextFactory> factory,
           dp::Viewport const & viewport, MapDataProvider const & model, Hints const & hints,
           double vs, double fontsScaleFactor, gui::TWidgetsInitInfo && info,
           location::TMyPositionModeChanged && myPositionModeChanged, bool allow3dBuildings,
           bool trafficEnabled, bool isolinesEnabled, bool blockTapEvents,
           bool showChoosePositionMark, std::vector<m2::TriangleD> && boundAreaTriangles,
           bool isRoutingActive, bool isAutozoomEnabled, bool simplifiedTrafficColors,
           std::optional<Arrow3dCustomDecl> arrow3dCustomDecl,
           OverlaysShowStatsCallback && overlaysShowStatsCallback,
           OnGraphicsContextInitialized && onGraphicsContextInitialized)
      : m_apiVersion(apiVersion)
      , m_factory(factory)
      , m_viewport(viewport)
      , m_model(model)
      , m_hints(hints)
      , m_vs(vs)
      , m_fontsScaleFactor(fontsScaleFactor)
      , m_info(std::move(info))
      , m_myPositionModeChanged(std::move(myPositionModeChanged))
      , m_allow3dBuildings(allow3dBuildings)
      , m_trafficEnabled(trafficEnabled)
      , m_isolinesEnabled(isolinesEnabled)
      , m_blockTapEvents(blockTapEvents)
      , m_showChoosePositionMark(showChoosePositionMark)
      , m_boundAreaTriangles(std::move(boundAreaTriangles))
      , m_isRoutingActive(isRoutingActive)
      , m_isAutozoomEnabled(isAutozoomEnabled)
      , m_simplifiedTrafficColors(simplifiedTrafficColors)
      , m_arrow3dCustomDecl(std::move(arrow3dCustomDecl))
      , m_overlaysShowStatsCallback(std::move(overlaysShowStatsCallback))
      , m_onGraphicsContextInitialized(std::move(onGraphicsContextInitialized))
    {}

    dp::ApiVersion m_apiVersion;
    ref_ptr<dp::GraphicsContextFactory> m_factory;
    dp::Viewport m_viewport;
    MapDataProvider m_model;
    Hints m_hints;
    double m_vs;
    double m_fontsScaleFactor;
    gui::TWidgetsInitInfo m_info;
    std::pair<location::EMyPositionMode, bool> m_initialMyPositionMode;
    location::TMyPositionModeChanged m_myPositionModeChanged;
    bool m_allow3dBuildings;
    bool m_trafficEnabled;
    bool m_isolinesEnabled;
    bool m_blockTapEvents;
    bool m_showChoosePositionMark;
    std::vector<m2::TriangleD> m_boundAreaTriangles;
    bool m_isRoutingActive;
    bool m_isAutozoomEnabled;
    bool m_simplifiedTrafficColors;
    std::optional<Arrow3dCustomDecl> m_arrow3dCustomDecl;
    OverlaysShowStatsCallback m_overlaysShowStatsCallback;
    OnGraphicsContextInitialized m_onGraphicsContextInitialized;
  };

  explicit DrapeEngine(Params && params);
  ~DrapeEngine();

  void RecoverSurface(int w, int h, bool recreateContextDependentResources);

  void Resize(int w, int h);
  void Invalidate() const;

  void SetVisibleViewport(m2::RectD const & rect) const;

  void AddTouchEvent(TouchEvent const & event) const;
  void Scale(double factor, m2::PointD const & pxPoint, bool isAnim) const;
  void Move(double factorX, double factorY, bool isAnim) const;
  void Scroll(double distanceX, double distanceY) const;
  void Rotate(double azimuth, bool isAnim) const;

  void ScaleAndSetCenter(m2::PointD const & centerPt, double scaleFactor, bool isAnim,
                         bool trackVisibleViewport) const;

  // If zoom == -1 then current zoom will not be changed.
  void SetModelViewCenter(m2::PointD const & centerPt, int zoom, bool isAnim,
                          bool trackVisibleViewport) const;
  void SetModelViewRect(m2::RectD const & rect, bool applyRotation, int zoom, bool isAnim,
                        bool useVisibleViewport) const;
  void SetModelViewAnyRect(m2::AnyRectD const & rect, bool isAnim, bool useVisibleViewport) const;

  using ModelViewChangedHandler = FrontendRenderer::ModelViewChangedHandler;
  void SetModelViewListener(ModelViewChangedHandler && fn);

#if defined(OMIM_OS_DESKTOP)
  using GraphicsReadyHandler = FrontendRenderer::GraphicsReadyHandler;
  void NotifyGraphicsReady(GraphicsReadyHandler const & fn, bool needInvalidate) const;
#endif

  void ClearUserMarksGroup(kml::MarkGroupId groupId) const;
  void ChangeVisibilityUserMarksGroup(kml::MarkGroupId groupId, bool isVisible) const;
  void UpdateUserMarks(UserMarksProvider const * provider, bool firstTime) const;
  void InvalidateUserMarks() const;

  void SetRenderingEnabled(ref_ptr<dp::GraphicsContextFactory> contextFactory = nullptr) const;
  void SetRenderingDisabled(bool destroySurface) const;
  void InvalidateRect(m2::RectD const & rect) const;
  void UpdateMapStyle() const;

  void SetCompassInfo(location::CompassInfo const & info) const;
  void SetGpsInfo(location::GpsInfo const & info, bool isNavigable,
                  location::RouteMatchingInfo const & routeInfo) const;
  void SwitchMyPositionNextMode() const;
  void LoseLocation() const;
  void StopLocationFollow() const;

  using TapEventInfoHandler = FrontendRenderer::TapEventInfoHandler;
  void SetTapEventInfoListener(TapEventInfoHandler && fn);
  using UserPositionChangedHandler = FrontendRenderer::UserPositionChangedHandler;
  void SetUserPositionListener(UserPositionChangedHandler && fn);

  void SelectObject(SelectionShape::ESelectedObject obj, m2::PointD const & pt,
                    FeatureID const & featureID, bool isAnim, bool isGeometrySelectionAllowed,
                    bool isSelectionShapeVisible) const;
  void DeselectObject() const;

  dp::DrapeID AddSubroute(SubrouteConstPtr subroute);
  void RemoveSubroute(dp::DrapeID subrouteId, bool deactivateFollowing) const;
  void FollowRoute(int preferredZoomLevel, int preferredZoomLevel3d, bool enableAutoZoom,
                   bool isArrowGlued) const;
  void DeactivateRouteFollowing() const;
  void SetSubrouteVisibility(dp::DrapeID subrouteId, bool isVisible) const;
  dp::DrapeID AddRoutePreviewSegment(m2::PointD const & startPt, m2::PointD const & finishPt);
  void RemoveRoutePreviewSegment(dp::DrapeID segmentId) const;
  void RemoveAllRoutePreviewSegments() const;

  void SetWidgetLayout(gui::TWidgetsLayoutInfo && info);

  void AllowAutoZoom(bool allowAutoZoom) const;

  void Allow3dMode(bool allowPerspectiveInNavigation, bool allow3dBuildings) const;
  void EnablePerspective() const;

  void UpdateGpsTrackPoints(std::vector<df::GpsTrackPoint> && toAdd,
                            std::vector<uint32_t> && toRemove) const;
  void ClearGpsTrackPoints() const;

  void EnableChoosePositionMode(bool enable, std::vector<m2::TriangleD> && boundAreaTriangles,
                                bool hasPosition, m2::PointD const & position);
  void BlockTapEvents(bool block) const;

  void SetKineticScrollEnabled(bool enabled);

  void OnEnterForeground() const;
  void OnEnterBackground();

  using TRequestSymbolsSizeCallback = std::function<void(std::map<std::string, m2::PointF> &&)>;

  void RequestSymbolsSize(std::vector<std::string> const & symbols,
                          TRequestSymbolsSizeCallback const & callback) const;

  void EnableTraffic(bool trafficEnabled) const;
  void UpdateTraffic(traffic::TrafficInfo const & info) const;
  void ClearTrafficCache(MwmSet::MwmId const & mwmId) const;
  void SetSimplifiedTrafficColors(bool simplified) const;

  void EnableTransitScheme(bool enable) const;
  void UpdateTransitScheme(TransitDisplayInfos && transitDisplayInfos) const;
  void ClearTransitSchemeCache(MwmSet::MwmId const & mwmId) const;
  void ClearAllTransitSchemeCache() const;

  void EnableIsolines(bool enable) const;

  void SetFontScaleFactor(double scaleFactor);

  void RunScenario(ScenarioManager::ScenarioData && scenarioData,
                   ScenarioManager::ScenarioCallback const & onStartFn,
                   ScenarioManager::ScenarioCallback const & onFinishFn) const;

  /// @name Custom features are features that we render in a different way.
  /// Value in the map shows if the feature is skipped in process of geometry generation.
  /// For all custom features (if they are overlays) statistics will be gathered.
  /// @todo Not used now, suspect that it was used for some Ads POIs.
  /// @{
  void SetCustomFeatures(df::CustomFeatures && ids) const;
  void RemoveCustomFeatures(MwmSet::MwmId const & mwmId) const;
  void RemoveAllCustomFeatures() const;
  /// @}

  void SetPosteffectEnabled(PostprocessRenderer::Effect effect, bool enabled) const;
  void EnableDebugRectRendering(bool enabled) const;

  void RunFirstLaunchAnimation() const;

  void ShowDebugInfo(bool shown) const;

  void UpdateVisualScale(double vs, bool needStopRendering);
  void UpdateMyPositionRoutingOffset(bool useDefault, int offsetY) const;

  location::EMyPositionMode GetMyPositionMode() const;

  void SetCustomArrow3d(std::optional<Arrow3dCustomDecl> arrow3dCustomDecl) const;

  dp::ApiVersion GetApiVersion() const { return m_frontend->GetApiVersion(); };

private:
  void AddUserEvent(drape_ptr<UserEvent> && e) const;
  void PostUserEvent(drape_ptr<UserEvent> && e) const;
  void ModelViewChanged(ScreenBase const & screen) const;
  void MyPositionModeChanged(location::EMyPositionMode mode, bool routingActive) const;
  void TapEvent(TapInfo const & tapInfo) const;
  void UserPositionChanged(m2::PointD const & position, bool hasPosition) const;

  void ResizeImpl(int32_t w, int32_t h);
  void RecacheGui(bool needResetOldGui);
  void RecacheMapShapes() const;

  dp::DrapeID GenerateDrapeID();

  static drape_ptr<UserMarkRenderParams> GenerateMarkRenderInfo(UserPointMark const * mark);
  static drape_ptr<UserLineRenderParams> GenerateLineRenderInfo(UserLineMark const * mark);

  drape_ptr<FrontendRenderer> m_frontend;
  drape_ptr<BackendRenderer> m_backend;
  drape_ptr<ThreadsCommutator> m_threadCommutator;
  drape_ptr<dp::TextureManager> m_textureManager;
  drape_ptr<RequestedTiles> m_requestedTiles;
  location::TMyPositionModeChanged m_myPositionModeChanged;

  dp::Viewport m_viewport;

  ModelViewChangedHandler m_modelViewChangedHandler;
  TapEventInfoHandler m_tapEventInfoHandler;
  UserPositionChangedHandler m_userPositionChangedHandler;

  gui::TWidgetsInitInfo m_widgetsInfo;
  gui::TWidgetsLayoutInfo m_widgetsLayout;

  bool m_choosePositionMode = false;
  bool m_kineticScrollEnabled = true;

  std::atomic<dp::DrapeID> m_drapeIdGenerator = 0;

  double m_startBackgroundTime = 0;

  friend class DrapeApi;
};
}  // namespace df
