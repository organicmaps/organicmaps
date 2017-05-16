#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/my_position_controller.hpp"

#include "drape_frontend/gui/drape_gui.hpp"

#include "platform/settings.hpp"

namespace df
{
DrapeEngine::DrapeEngine(Params && params)
  : m_myPositionModeChanged(move(params.m_myPositionModeChanged))
  , m_viewport(move(params.m_viewport))
{
  VisualParams::Init(params.m_vs, df::CalculateTileSize(m_viewport.GetWidth(), m_viewport.GetHeight()));

  df::VisualParams::Instance().SetFontScale(params.m_fontsScaleFactor);

  gui::DrapeGui & guiSubsystem = gui::DrapeGui::Instance();
  guiSubsystem.SetLocalizator(bind(&StringsBundle::GetString, params.m_stringsBundle.get(), _1));
  guiSubsystem.SetSurfaceSize(m2::PointF(m_viewport.GetWidth(), m_viewport.GetHeight()));

  m_textureManager = make_unique_dp<dp::TextureManager>();
  m_threadCommutator = make_unique_dp<ThreadsCommutator>();
  m_requestedTiles = make_unique_dp<RequestedTiles>();

  location::EMyPositionMode mode = params.m_initialMyPositionMode.first;
  if (!params.m_initialMyPositionMode.second && !settings::Get(settings::kLocationStateMode, mode))
  {
    mode = location::PendingPosition;
  }
  else if (mode == location::FollowAndRotate)
  {
    // If the screen rect setting in follow and rotate mode is missing or invalid, it could cause invalid animations,
    // so the follow and rotate mode should be discarded.
    m2::AnyRectD rect;
    if (!(settings::Get("ScreenClipRect", rect) && df::GetWorldRect().IsRectInside(rect.GetGlobalRect())))
      mode = location::Follow;
  }

  double timeInBackground = 0.0;
  double lastEnterBackground = 0.0;
  if (settings::Get("LastEnterBackground", lastEnterBackground))
    timeInBackground = my::Timer::LocalTime() - lastEnterBackground;

  MyPositionController::Params mpParams(mode,
                                        timeInBackground,
                                        params.m_hints,
                                        params.m_isRoutingActive,
                                        params.m_isAutozoomEnabled,
                                        bind(&DrapeEngine::MyPositionModeChanged, this, _1, _2));

  FrontendRenderer::Params frParams(params.m_apiVersion,
                                    make_ref(m_threadCommutator),
                                    params.m_factory,
                                    make_ref(m_textureManager),
                                    move(mpParams),
                                    m_viewport,
                                    bind(&DrapeEngine::ModelViewChanged, this, _1),
                                    bind(&DrapeEngine::TapEvent, this, _1),
                                    bind(&DrapeEngine::UserPositionChanged, this, _1),
                                    make_ref(m_requestedTiles),
                                    move(params.m_overlaysShowStatsCallback),
                                    params.m_allow3dBuildings,
                                    params.m_trafficEnabled,
                                    params.m_blockTapEvents);

  m_frontend = make_unique_dp<FrontendRenderer>(move(frParams));

  BackendRenderer::Params brParams(params.m_apiVersion,
                                   frParams.m_commutator,
                                   frParams.m_oglContextFactory,
                                   frParams.m_texMng,
                                   params.m_model,
                                   params.m_model.UpdateCurrentCountryFn(),
                                   make_ref(m_requestedTiles),
                                   params.m_allow3dBuildings,
                                   params.m_trafficEnabled,
                                   params.m_simplifiedTrafficColors);

  m_backend = make_unique_dp<BackendRenderer>(move(brParams));

  m_widgetsInfo = move(params.m_info);

  RecacheGui(false);
  RecacheMapShapes();

  if (params.m_showChoosePositionMark)
    EnableChoosePositionMode(true, move(params.m_boundAreaTriangles), false, m2::PointD());

  ResizeImpl(m_viewport.GetWidth(), m_viewport.GetHeight());
}

DrapeEngine::~DrapeEngine()
{
  // Call Teardown explicitly! We must wait for threads completion.
  m_frontend->Teardown();
  m_backend->Teardown();

  // Reset thread commutator, it stores BaseRenderer pointers.
  m_threadCommutator.reset();

  // Reset pointers to FrontendRenderer and BackendRenderer.
  m_frontend.reset();
  m_backend.reset();

  gui::DrapeGui::Instance().Destroy();
  m_textureManager->Release();
}

void DrapeEngine::Update(int w, int h)
{
  if (m_choosePositionMode)
  {
    m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<ShowChoosePositionMarkMessage>(),
                                    MessagePriority::High);
  }
  RecacheGui(false);

  RecacheMapShapes();

  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<RecoverGLResourcesMessage>(),
                                  MessagePriority::High);

  ResizeImpl(w, h);
}

void DrapeEngine::Resize(int w, int h)
{
  ASSERT_GREATER(w, 0, ());
  ASSERT_GREATER(h, 0, ());
  if (m_viewport.GetHeight() != static_cast<uint32_t>(h) ||
      m_viewport.GetWidth() != static_cast<uint32_t>(w))
    ResizeImpl(w, h);
}

void DrapeEngine::SetVisibleViewport(m2::RectD const & rect) const
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SetVisibleViewportMessage>(rect),
                                  MessagePriority::Normal);
}

void DrapeEngine::Invalidate()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<InvalidateMessage>(),
                                  MessagePriority::High);
}

void DrapeEngine::AddTouchEvent(TouchEvent const & event)
{
  AddUserEvent(make_unique_dp<TouchEvent>(event));
}

void DrapeEngine::Scale(double factor, m2::PointD const & pxPoint, bool isAnim)
{
  AddUserEvent(make_unique_dp<ScaleEvent>(factor, pxPoint, isAnim));
}

void DrapeEngine::SetModelViewCenter(m2::PointD const & centerPt, int zoom, bool isAnim)
{
  AddUserEvent(make_unique_dp<SetCenterEvent>(centerPt, zoom, isAnim));
}

void DrapeEngine::SetModelViewRect(m2::RectD const & rect, bool applyRotation, int zoom, bool isAnim)
{
  AddUserEvent(make_unique_dp<SetRectEvent>(rect, applyRotation, zoom, isAnim));
}

void DrapeEngine::SetModelViewAnyRect(m2::AnyRectD const & rect, bool isAnim)
{
  AddUserEvent(make_unique_dp<SetAnyRectEvent>(rect, isAnim));
}

void DrapeEngine::ClearUserMarksLayer(size_t layerId)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ClearUserMarkLayerMessage>(layerId),
                                  MessagePriority::Normal);
}

void DrapeEngine::ChangeVisibilityUserMarksLayer(size_t layerId, bool isVisible)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeUserMarkLayerVisibilityMessage>(layerId, isVisible),
                                  MessagePriority::Normal);
}

void DrapeEngine::UpdateUserMarksLayer(size_t layerId, UserMarksProvider * provider)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<UpdateUserMarkLayerMessage>(layerId, provider),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetRenderingEnabled(ref_ptr<dp::OGLContextFactory> contextFactory)
{
  m_backend->SetRenderingEnabled(contextFactory);
  m_frontend->SetRenderingEnabled(contextFactory);

  LOG(LDEBUG, ("Rendering enabled"));
}

void DrapeEngine::SetRenderingDisabled(bool const destroyContext)
{
  m_frontend->SetRenderingDisabled(destroyContext);
  m_backend->SetRenderingDisabled(destroyContext);

  LOG(LDEBUG, ("Rendering disabled"));
}

void DrapeEngine::InvalidateRect(m2::RectD const & rect)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<InvalidateRectMessage>(rect),
                                  MessagePriority::High);
}

void DrapeEngine::UpdateMapStyle()
{
  // Update map style.
  {
    UpdateMapStyleMessage::Blocker blocker;
    m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                    make_unique_dp<UpdateMapStyleMessage>(blocker),
                                    MessagePriority::High);
    blocker.Wait();
  }

  // Recache gui after updating of style.
  RecacheGui(false);
}

void DrapeEngine::RecacheMapShapes()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<MapShapesRecacheMessage>(),
                                  MessagePriority::Normal);
}

void DrapeEngine::RecacheGui(bool needResetOldGui)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<GuiRecacheMessage>(m_widgetsInfo, needResetOldGui),
                                  MessagePriority::High);
}

void DrapeEngine::AddUserEvent(drape_ptr<UserEvent> && e)
{
  m_frontend->AddUserEvent(move(e));
}

void DrapeEngine::ModelViewChanged(ScreenBase const & screen)
{
  if (m_modelViewChanged != nullptr)
    m_modelViewChanged(screen);
}

void DrapeEngine::MyPositionModeChanged(location::EMyPositionMode mode, bool routingActive)
{
  settings::Set(settings::kLocationStateMode, mode);
  if (m_myPositionModeChanged != nullptr)
    m_myPositionModeChanged(mode, routingActive);
}

void DrapeEngine::TapEvent(TapInfo const & tapInfo)
{
  if (m_tapListener != nullptr)
    m_tapListener(tapInfo);
}

void DrapeEngine::UserPositionChanged(m2::PointD const & position)
{
  if (m_userPositionChanged != nullptr)
    m_userPositionChanged(position);
}

void DrapeEngine::ResizeImpl(int w, int h)
{
  gui::DrapeGui::Instance().SetSurfaceSize(m2::PointF(w, h));
  m_viewport.SetViewport(0, 0, w, h);
  AddUserEvent(make_unique_dp<ResizeEvent>(w, h));
}

void DrapeEngine::SetCompassInfo(location::CompassInfo const & info)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<CompassInfoMessage>(info),
                                  MessagePriority::High);
}

void DrapeEngine::SetGpsInfo(location::GpsInfo const & info, bool isNavigable, const location::RouteMatchingInfo & routeInfo)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<GpsInfoMessage>(info, isNavigable, routeInfo),
                                  MessagePriority::High);
}

void DrapeEngine::SwitchMyPositionNextMode()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeMyPositionModeMessage>(ChangeMyPositionModeMessage::SwitchNextMode),
                                  MessagePriority::High);
}

void DrapeEngine::LoseLocation()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeMyPositionModeMessage>(ChangeMyPositionModeMessage::LoseLocation),
                                  MessagePriority::High);
}

void DrapeEngine::StopLocationFollow()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeMyPositionModeMessage>(ChangeMyPositionModeMessage::StopFollowing),
                                  MessagePriority::High);
}

void DrapeEngine::FollowRoute(int preferredZoomLevel, int preferredZoomLevel3d, bool enableAutoZoom)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<FollowRouteMessage>(preferredZoomLevel, preferredZoomLevel3d, enableAutoZoom),
                                  MessagePriority::High);
}

void DrapeEngine::SetModelViewListener(TModelViewListenerFn && fn)
{
  m_modelViewChanged = move(fn);
}

void DrapeEngine::SetTapEventInfoListener(TTapEventInfoFn && fn)
{
  m_tapListener = move(fn);
}

void DrapeEngine::SetUserPositionListener(TUserPositionChangedFn && fn)
{
  m_userPositionChanged = move(fn);
}

FeatureID DrapeEngine::GetVisiblePOI(m2::PointD const & glbPoint)
{
  FeatureID result;
  BaseBlockingMessage::Blocker blocker;
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<FindVisiblePOIMessage>(blocker, glbPoint, result),
                                  MessagePriority::High);

  blocker.Wait();
  return result;
}

void DrapeEngine::SelectObject(SelectionShape::ESelectedObject obj, m2::PointD const & pt, FeatureID const & featureId, bool isAnim)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SelectObjectMessage>(obj, pt, featureId, isAnim),
                                  MessagePriority::High);
}

void DrapeEngine::DeselectObject()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SelectObjectMessage>(SelectObjectMessage::DismissTag()),
                                  MessagePriority::High);
}

SelectionShape::ESelectedObject DrapeEngine::GetSelectedObject()
{
  SelectionShape::ESelectedObject object;
  BaseBlockingMessage::Blocker blocker;
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<GetSelectedObjectMessage>(blocker, object),
                                  MessagePriority::High);

  blocker.Wait();
  return object;
}

bool DrapeEngine::GetMyPosition(m2::PointD & myPosition)
{
  bool hasPosition = false;
  BaseBlockingMessage::Blocker blocker;
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<GetMyPositionMessage>(blocker, hasPosition, myPosition),
                                  MessagePriority::High);

  blocker.Wait();
  return hasPosition;
}

void DrapeEngine::AddRoute(m2::PolylineD const & routePolyline, vector<double> const & turns,
                           df::ColorConstant color, vector<traffic::SpeedGroup> const & traffic,
                           df::RoutePattern pattern)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<AddRouteMessage>(routePolyline, turns, color, traffic, pattern),
                                  MessagePriority::Normal);
}

void DrapeEngine::RemoveRoute(bool deactivateFollowing)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<RemoveRouteMessage>(deactivateFollowing),
                                  MessagePriority::Normal);
}

void DrapeEngine::DeactivateRouteFollowing()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<DeactivateRouteFollowingMessage>(),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetRoutePoint(m2::PointD const & position, bool isStart, bool isValid)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<CacheRouteSignMessage>(position, isStart, isValid),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetWidgetLayout(gui::TWidgetsLayoutInfo && info)
{
  m_widgetsLayout = move(info);
  for (auto const & layout : m_widgetsLayout)
  {
    auto const itInfo = m_widgetsInfo.find(layout.first);
    if (itInfo != m_widgetsInfo.end())
      itInfo->second.m_pixelPivot = layout.second;
  }
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<GuiLayerLayoutMessage>(m_widgetsLayout),
                                  MessagePriority::Normal);
}

void DrapeEngine::AllowAutoZoom(bool allowAutoZoom)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<AllowAutoZoomMessage>(allowAutoZoom),
                                  MessagePriority::Normal);
}

void DrapeEngine::Allow3dMode(bool allowPerspectiveInNavigation, bool allow3dBuildings)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<Allow3dBuildingsMessage>(allow3dBuildings),
                                  MessagePriority::Normal);

  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<Allow3dModeMessage>(allowPerspectiveInNavigation, allow3dBuildings),
                                  MessagePriority::Normal);
}

void DrapeEngine::EnablePerspective()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<EnablePerspectiveMessage>(),
                                  MessagePriority::Normal);
}

void DrapeEngine::UpdateGpsTrackPoints(vector<df::GpsTrackPoint> && toAdd, vector<uint32_t> && toRemove)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<UpdateGpsTrackPointsMessage>(move(toAdd), move(toRemove)),
                                  MessagePriority::Normal);
}

void DrapeEngine::ClearGpsTrackPoints()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ClearGpsTrackPointsMessage>(),
                                  MessagePriority::Normal);
}

void DrapeEngine::EnableChoosePositionMode(bool enable, vector<m2::TriangleD> && boundAreaTriangles,
                                           bool hasPosition, m2::PointD const & position)
{
  m_choosePositionMode = enable;
  bool kineticScroll = m_kineticScrollEnabled;
  if (enable)
  {
    StopLocationFollow();
    m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<ShowChoosePositionMarkMessage>(),
                                    MessagePriority::High);
    kineticScroll = false;
  }
  else
  {
    RecacheGui(true);
  }
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SetAddNewPlaceModeMessage>(enable, move(boundAreaTriangles),
                                                                            kineticScroll, hasPosition, position),
                                  MessagePriority::High);
}

void DrapeEngine::BlockTapEvents(bool block)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<BlockTapEventsMessage>(block),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetKineticScrollEnabled(bool enabled)
{
  m_kineticScrollEnabled = enabled;
  if (m_choosePositionMode)
    return;

  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SetKineticScrollEnabledMessage>(m_kineticScrollEnabled),
                                  MessagePriority::High);
}

void DrapeEngine::SetTimeInBackground(double time)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SetTimeInBackgroundMessage>(time),
                                  MessagePriority::High);
}

void DrapeEngine::SetDisplacementMode(int mode)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<SetDisplacementModeMessage>(mode),
                                  MessagePriority::Normal);
}

void DrapeEngine::RequestSymbolsSize(vector<string> const & symbols,
                                     TRequestSymbolsSizeCallback const & callback)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<RequestSymbolsSizeMessage>(symbols, callback),
                                  MessagePriority::Normal);
}

void DrapeEngine::EnableTraffic(bool trafficEnabled)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<EnableTrafficMessage>(trafficEnabled),
                                  MessagePriority::Normal);
}

void DrapeEngine::UpdateTraffic(traffic::TrafficInfo const & info)
{
  if (info.GetColoring().empty())
    return;

#ifdef DEBUG
  for (auto const & segmentPair : info.GetColoring())
    ASSERT_NOT_EQUAL(segmentPair.second, traffic::SpeedGroup::Unknown, ());
#endif

  df::TrafficSegmentsColoring segmentsColoring;
  segmentsColoring.emplace(info.GetMwmId(), info.GetColoring());

  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<UpdateTrafficMessage>(move(segmentsColoring)),
                                  MessagePriority::Normal);
}

void DrapeEngine::ClearTrafficCache(MwmSet::MwmId const & mwmId)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<ClearTrafficDataMessage>(mwmId),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetSimplifiedTrafficColors(bool simplified)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<SetSimplifiedTrafficColorsMessage>(simplified),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetFontScaleFactor(double scaleFactor)
{
  double const kMinScaleFactor = 0.5;
  double const kMaxScaleFactor = 2.0;

  scaleFactor = my::clamp(scaleFactor, kMinScaleFactor, kMaxScaleFactor);

  VisualParams::Instance().SetFontScale(scaleFactor);
}

void DrapeEngine::RunScenario(ScenarioManager::ScenarioData && scenarioData,
                              ScenarioManager::ScenarioCallback const & onStartFn,
                              ScenarioManager::ScenarioCallback const & onFinishFn)
{
  auto const & manager = m_frontend->GetScenarioManager();
  if (manager != nullptr)
    manager->RunScenario(move(scenarioData), onStartFn, onFinishFn);
}

void DrapeEngine::AddCustomSymbols(CustomSymbols && symbols)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<AddCustomSymbolsMessage>(std::move(symbols)),
                                  MessagePriority::Normal);
}

void DrapeEngine::RemoveCustomSymbols(MwmSet::MwmId const & mwmId)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<RemoveCustomSymbolsMessage>(mwmId),
                                  MessagePriority::Normal);
}

void DrapeEngine::RemoveAllCustomSymbols()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<RemoveCustomSymbolsMessage>(),
                                  MessagePriority::Normal);
}

} // namespace df
