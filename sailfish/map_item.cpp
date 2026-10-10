#include "sailfish/map_item.hpp"

#include "sailfish/app_settings.hpp"
#include "sailfish/bookmarks_model.hpp"
#include "sailfish/countries_model.hpp"
#include "sailfish/elevation.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/helpers.hpp"
#include "sailfish/maps_storage.hpp"
#include "sailfish/place_page.hpp"
#include "sailfish/routing.hpp"

#include "map/framework.hpp"
#include "map/gps_tracker.hpp"

#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/map_style.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"
#include "storage/storage_helpers.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"

#include <QCompass>
#include <QDateTime>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QPointer>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QScreen>
#include <QTouchEvent>

#include <algorithm>
#include <optional>
#include <utility>

namespace sailfish
{
namespace
{
// Bit mask of the layers tapped in the layers sheet.
std::string_view constexpr kSeenLayersSetting = "SailfishSeenLayers";
// Below it, in m/s, the speed of a standing phone is GPS noise.
double constexpr kMinShownSpeed = 0.5;
// QCompassReading::calibrationLevel() thresholds of the two calibration notices.
double constexpr kCalibrationRecommended = 0.67;
double constexpr kCalibrationRequired = 0.34;

static_assert(MapItem::PendingPosition == static_cast<int>(location::PendingPosition));
static_assert(MapItem::NotFollowNoPosition == static_cast<int>(location::NotFollowNoPosition));
static_assert(MapItem::NotFollow == static_cast<int>(location::NotFollow));
static_assert(MapItem::Follow == static_cast<int>(location::Follow));
static_assert(MapItem::FollowAndRotate == static_cast<int>(location::FollowAndRotate));

// Drape presents into a power-of-two framebuffer and reports the used part as a normalized
// rect, so the texture wrapper is recreated whenever the framebuffer or its size changes.
class MapTextureNode : public QSGSimpleTextureNode
{
public:
  MapTextureNode()
  {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    // setTexture() then deletes the previous wrapper.
    setOwnsTexture(true);
  }

  void Update(QQuickWindow * window, GLuint id, QSize const & textureSize, QSize const & usedSize)
  {
    if (!texture() || id != m_id || textureSize != m_textureSize)
    {
      setTexture(window->createTextureFromId(id, textureSize));
      m_id = id;
      m_textureSize = textureSize;
    }
    setSourceRect(0, 0, usedSize.width(), usedSize.height());
  }

private:
  GLuint m_id = 0;
  QSize m_textureSize;
};
}  // namespace

MapItem::MapItem(QQuickItem * parent)
  : QQuickItem(parent)
  , m_framework(GetFramework())
  , m_locationService(CreateDesktopLocationService(*this))
  , m_placePage(std::make_unique<PlacePage>(m_framework))
  , m_routing(std::make_unique<Routing>(m_framework))
  , m_compass(new QCompass(this))
{
  m_compass->setSkipDuplicates(true);
  connect(m_compass, &QCompass::readingChanged, this, &MapItem::OnCompassReading);

  setFlag(ItemHasContents);
  setAcceptedMouseButtons(Qt::LeftButton);

  connect(this, &QQuickItem::windowChanged, this, &MapItem::OnWindowChanged);
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &MapItem::OnApplicationStateChanged);

  m_storageSlot = m_framework.GetStorage().Subscribe([this](storage::CountryId const &) {
    UpdateCurrentCountry();
  }, [this](storage::CountryId const &, downloader::Progress const &) { UpdateCurrentCountry(); });

  // Drape renders on its own threads, so poll for new frames like the desktop map widget.
  m_updateTimer.setInterval(1000 / 60);
  connect(&m_updateTimer, &QTimer::timeout, this, &QQuickItem::update);
}

MapItem::~MapItem()
{
  // The listeners capture this.
  m_framework.SetTrackRecordingUpdateHandler(nullptr);
  m_framework.GetIsolinesManager().SetStateListener({});
  m_framework.SetCurrentCountryChangedListener({});
  m_framework.SetMyPositionModeListener(nullptr);
  m_framework.GetStorage().Unsubscribe(m_storageSlot);
  m_locationService->Stop();
  if (!m_contextFactory)
    return;

  m_framework.EnterBackground();
  m_framework.SetRenderingDisabled(true);
  m_contextFactory->PrepareToShutdown();
  m_framework.DestroyDrapeEngine();
  m_contextFactory.reset();
}

void MapItem::zoomIn()
{
  m_framework.Scale(Framework::SCALE_MAG, true);
}

void MapItem::zoomOut()
{
  m_framework.Scale(Framework::SCALE_MIN, true);
}

void MapItem::switchMyPositionMode()
{
  if (m_contextFactory)
    m_framework.SwitchMyPositionNextMode();
}

int MapItem::enabledLayers() const
{
  int mask = 0;
  auto const set = [&mask](Layer layer, bool enabled)
  {
    if (enabled)
      mask |= 1 << layer;
  };
  set(Outdoors, Framework::LoadOutdoorsEnabled());
  set(Isolines, Framework::LoadIsolinesEnabled());
  set(Hiking, Framework::IsHikingEnabled());
  set(Cycling, Framework::IsCyclingEnabled());
  set(Subway, Framework::LoadTransitSchemeEnabled());
  set(Satellite, Framework::IsBackgroundTilesEnabled());
  return mask;
}

int MapItem::newLayers() const
{
  int constexpr kNeverNew = 1 << Isolines | 1 << Subway;
  return ~(LoadSetting(kSeenLayersSetting, 0) | kNeverNew);
}

void MapItem::setLayerEnabled(int layer, bool enabled)
{
  settings::Set(kSeenLayersSetting, LoadSetting(kSeenLayersSetting, 0) | 1 << layer);
  switch (layer)
  {
  case Outdoors:
  {
    Framework::SaveOutdoorsEnabled(enabled);
    m_framework.SetMapStyle(BaseMapStyle(MapStyleIsDark(m_framework.GetMapStyle()), enabled));
    break;
  }
  case Isolines:
    m_framework.GetIsolinesManager().SetEnabled(enabled);
    Framework::SaveIsolinesEnabled(enabled);
    break;
  case Satellite: m_framework.SetBackgroundTilesEnabled(enabled); break;
  case Hiking: m_framework.SetHikingEnabled(enabled); break;
  case Cycling: m_framework.SetCyclingEnabled(enabled); break;
  case Subway:
    m_framework.GetTransitManager().EnableTransitSchemeMode(enabled);
    Framework::SaveTransitSchemeEnabled(enabled);
    break;
  default: ASSERT(false, ("Unknown layer", layer)); return;
  }
  emit layersChanged();
}

void MapItem::OnMyPositionModeChanged(location::EMyPositionMode mode)
{
  // Drape asks for a position by entering PendingPosition, either on start or from the button.
  if (mode == location::PendingPosition && !m_inBackground)
    m_locationService->Start();

  if (mode != m_myPositionMode)
  {
    m_myPositionMode = mode;
    emit myPositionModeChanged();
  }
}

void MapItem::OnLocationError(location::TLocationError errorCode)
{
  LOG(LWARNING, ("Location error:", errorCode));
  m_framework.OnLocationError(errorCode);
  // Timeouts just mean no fix yet, e.g. indoors.
  bool const disabled =
      errorCode == location::EDenied || errorCode == location::EGPSIsOff || errorCode == location::ENotSupported;
  if (!std::exchange(m_locationLost, true))
    emit locationLostChanged();
  if (disabled && !std::exchange(m_locationErrorShown, true))
    emit notice(Localized("location_is_disabled_long_text"));
}

void MapItem::OnLocationUpdated(location::GpsInfo const & info)
{
  m_locationErrorShown = false;
  bool const regained = std::exchange(m_locationLost, false);
  if (regained)
    emit locationLostChanged();
  m_hasAltitude = info.HasAltitude();
  m_altitude = info.m_altitude;
  m_speed = info.m_speed;
  // The cover shows it while the app is put aside; the address lookup needn't follow every fix.
  if (m_inBackground && QDateTime::currentMSecsSinceEpoch() - m_positionInfoMs > 10000)
    UpdatePositionInfo();
  m_framework.OnLocationUpdate(info);
  GpsTracker::Instance().OnLocationUpdated(info);
  if (regained || !std::exchange(m_hadPosition, true))
    emit BookmarksNotifier::Instance().positionFound();
  m_placePage->UpdateDistance();
  m_placePage->UpdateMyPosition(info.HasAltitude(), info.m_altitude, info.m_speed);
  m_routing->UpdateNavigation(info.m_speed);
}

bool MapItem::trackRecording() const
{
  return m_framework.IsTrackRecordingEnabled();
}

void MapItem::startTrackRecording()
{
  m_framework.StartTrackRecording();
  // Recording needs fixes even when the my position button is off.
  m_locationService->Start();
  WatchRecording();
  emit trackRecordingChanged();
}

void MapItem::stopTrackRecording()
{
  m_framework.SetTrackRecordingUpdateHandler(nullptr);
  m_framework.StopTrackRecording();
  if (m_inBackground && !m_routing->navigating())
    m_locationService->Stop();
  emit trackRecordingChanged();
}

bool MapItem::saveAndStopTrackRecording()
{
  // Stopping discards the recording.
  bool const saved = !m_framework.IsTrackRecordingEmpty();
  if (saved)
    m_framework.SaveTrackRecordingWithName({});
  stopTrackRecording();
  return saved;
}

void MapItem::WatchRecording()
{
  if (!m_framework.IsTrackRecordingEnabled())
    return;
  // Called on the GUI thread with every recorded point, and right away. SafeCallback posts a copy of the
  // handler, which can run after this item is gone.
  m_framework.SetTrackRecordingUpdateHandler([self = QPointer<MapItem>(this), this](TrackStatistics const & stats)
  {
    if (!self)
      return;
    m_recordingSummary = JoinDetails(
        {QString::fromStdString(stats.GetFormattedLength()), FormatDuration(static_cast<long>(stats.m_duration))});
    m_recordingElevation.clear();
    if (!m_framework.IsTrackRecordingEmpty())
    {
      m_recordingElevation =
          ElevationChart(Framework::GetTrackRecordingElevationInfo(), stats.m_minElevation, stats.m_maxElevation);
    }
    emit recordingStatsChanged();
  });
}

QString MapItem::myPositionShareText() const
{
  auto const position = m_framework.GetCurrentPosition();
  if (!position)
    return {};
  return QString::fromStdString(m_framework.GetShareDataForMyPosition(mercator::ToLatLon(*position)).m_text);
}

void MapItem::startChoosingPosition(bool business)
{
  std::optional<m2::PointD> position;
  if (m_framework.HasPlacePageInfo())
    position = m_framework.GetCurrentPlacePageInfo().GetMercator();
  m_placePage->close();
  m_framework.BlockTapEvents(true);
  m_framework.EnableChoosePositionMode(true, business /* enableBounds */, position ? &*position : nullptr,
                                       true /* shouldChangeViewport */);
  m_choosingPosition = true;
  emit choosingPositionChanged();
}

void MapItem::stopChoosingPosition()
{
  if (!m_choosingPosition)
    return;
  m_framework.EnableChoosePositionMode(false, false /* enableBounds */, nullptr, false /* shouldChangeViewport */);
  m_framework.BlockTapEvents(false);
  m_choosingPosition = false;
  emit choosingPositionChanged();
}

QVariantList MapItem::confirmChosenPosition(bool requireMaps)
{
  // Taken now: the viewport can still move while the category is picked.
  auto const center = m_framework.GetViewportCenter();
  if (requireMaps &&
      !storage::IsPointCoveredByDownloadedMaps(center, m_framework.GetStorage(), m_framework.GetCountryInfoGetter()))
    return {};
  stopChoosingPosition();
  auto const latLon = mercator::ToLatLon(center);
  return {latLon.m_lat, latLon.m_lon};
}

void MapItem::OnCurrentCountryChanged(std::string const & countryId)
{
  m_currentCountryId = countryId;
  UpdateCurrentCountry();

  // Auto-download only on Wi-Fi, in that region and with enough space.
  if (countryId.empty() || !AppSettings::IsAutoDownloadEnabled() || MapsStorage::IsLocked())
    return;
  auto & storage = m_framework.GetStorage();
  if (MapAttrs(storage, countryId).m_status != storage::NodeStatus::NotDownloaded)
    return;
  auto const position = m_framework.GetCurrentPosition();
  if (!position || m_framework.GetCountryInfoGetter().GetRegionCountryId(*position) != countryId)
    return;
  if (!AppSettings::IsOnWifi())
    return;
  if (storage::IsEnoughSpaceForDownload(countryId, storage))
    storage.DownloadNode(countryId);
}

QVariantMap MapItem::mapToUpdateForEditing() const
{
  auto const center = m_framework.GetViewportCenter();
  if (m_framework.CanEditMapForPosition(center))
    return {};
  auto const country =
      MissingMapInfo(m_framework.GetStorage(), m_framework.GetCountryInfoGetter().GetRegionCountryId(center), true);
  return country.value("outdated").toBool() ? country : QVariantMap();
}

void MapItem::downloadMap(QString const & countryId)
{
  DownloadMap(m_framework.GetStorage(), countryId.toStdString());
}

bool MapItem::needUpdateForRoutes() const
{
  return m_framework.NeedUpdateForRoutes();
}

bool MapItem::isolinesNeedZoom() const
{
  auto const & manager = m_framework.GetIsolinesManager();
  return !manager.IsVisible() && manager.GetState() == IsolinesManager::IsolinesState::Enabled;
}

void MapItem::UpdatePositionInfo()
{
  m_positionInfoMs = QDateTime::currentMSecsSinceEpoch();
  QVariantMap info;
  if (auto const position = m_framework.GetCurrentPosition())
  {
    auto const latLon = mercator::ToLatLon(*position);
    info["address"] = QString::fromStdString(m_framework.GetAddressAtPoint(*position).FormatAddress());
    info["coordinates"] = FormatLatLon(latLon.m_lat, latLon.m_lon);
    if (m_hasAltitude)
      info["altitude"] = FormatAltitude(m_altitude);
    if (m_speed > kMinShownSpeed)
      info["speed"] = FormatSpeed(m_speed);
  }
  if (info != m_positionInfo)
  {
    m_positionInfo = info;
    emit positionInfoChanged();
  }
}

void MapItem::UpdateCurrentCountry()
{
  auto const country = MissingMapInfo(m_framework.GetStorage(), m_currentCountryId);
  if (country != m_currentCountry)
  {
    m_currentCountry = country;
    emit currentCountryChanged();
  }
}

void MapItem::cancelMap(QString const & countryId)
{
  m_framework.GetStorage().CancelDownloadNode(countryId.toStdString());
}

void MapItem::OnCompassReading()
{
  auto const * reading = m_compass->reading();
  if (!m_contextFactory || !reading || !window() || !window()->screen())
    return;

  // The azimuth is measured at the top of the device, so turn it with the UI. Magnetic declination is not
  // corrected.
  QScreen const * screen = window()->screen();
  int const rotation = screen->angleBetween(screen->nativeOrientation(), window()->contentOrientation());
  location::CompassInfo info;
  info.m_bearing = ang::AngleIn2PI(math::DegToRad(static_cast<double>(reading->azimuth() + rotation)));
  m_framework.OnCompassUpdate(info);
  m_placePage->SetNorth(info.m_bearing);

  // Warn once per session. Sensors without a calibration level report zero.
  auto const calibration = reading->calibrationLevel();
  if (calibration > 0 && calibration < kCalibrationRecommended && !std::exchange(m_calibrationShown, true))
  {
    emit notice(Localized(calibration < kCalibrationRequired ? "compass_calibration_required"
                                                             : "compass_calibration_recommended"));
  }
}

void MapItem::OnWindowChanged(QQuickWindow * window)
{
  if (window)
    CreateEngine();
}

void MapItem::OnApplicationStateChanged(Qt::ApplicationState state)
{
  // Sailfish keeps a minimized app running as its cover; stop drawing until it is active again.
  bool const inBackground = state != Qt::ApplicationActive;
  if (!m_contextFactory || inBackground == m_inBackground)
    return;

  m_inBackground = inBackground;
  LOG(LINFO, (inBackground ? "Entering background" : "Entering foreground"));
  if (inBackground)
  {
    UpdatePositionInfo();
    m_updateTimer.stop();
    // Track recording and navigation need location in the background.
    if (!m_framework.IsTrackRecordingEnabled() && !m_routing->navigating())
      m_locationService->Stop();
    m_compass->stop();
    m_framework.SetRenderingDisabled(false /* destroySurface */);
    m_framework.EnterBackground();
  }
  else
  {
    m_framework.EnterForeground();
    m_framework.SetRenderingEnabled();
    m_updateTimer.start();
    if (m_myPositionMode != location::NotFollowNoPosition)
      m_locationService->Start();
    m_compass->start();
  }
}

void MapItem::CreateEngine()
{
  if (m_contextFactory || !window() || width() <= 0 || height() <= 0)
    return;

  QOpenGLContext * shareContext = QOpenGLContext::globalShareContext();
  CHECK(shareContext, ("Qt::AA_ShareOpenGLContexts must be set before the application is created"));

  QScreen * screen = window()->screen();
  qreal const dpi = screen ? screen->physicalDotsPerInch() : 0;
  m_visualScale = dpi > 0 ? df::DPI2VS(dpi) : 2.0;
  LOG(LINFO, ("Screen DPI:", dpi, "visual scale:", m_visualScale));

  m_contextFactory = std::make_unique<qt::common::QtOGLContextFactory>(shareContext);

  Framework::DrapeCreationParams p;
  p.m_apiVersion = dp::ApiVersion::OpenGLES3;
  p.m_surfaceWidth = static_cast<int>(width());
  p.m_surfaceHeight = static_cast<int>(height());
  p.m_visualScale = static_cast<float>(m_visualScale);

  m_skin = std::make_unique<gui::Skin>(gui::ResolveGuiSkinFile("default"), m_visualScale);
  m_skin->Resize(p.m_surfaceWidth, p.m_surfaceHeight);
  m_skin->ForEach([&p](gui::EWidget widget, gui::Position const & pos) { p.m_widgetsInitInfo[widget] = pos; });

  m_framework.SetMyPositionModeListener([this](location::EMyPositionMode mode, bool /* routingActive */)
  { OnMyPositionModeChanged(mode); });
  m_framework.GetIsolinesManager().SetStateListener([this](IsolinesManager::IsolinesState state)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [self = QPointer<MapItem>(this), state]
    {
      if (!self)
        return;
      if (state == IsolinesManager::IsolinesState::NoData)
        emit self->notice(Localized("isolines_location_error_dialog"));
      else if (state == IsolinesManager::IsolinesState::ExpiredData)
        emit self->isolinesNeedMaps();
    });
  });
  m_framework.SetCurrentCountryChangedListener([this](storage::CountryId const & countryId)
  { OnCurrentCountryChanged(countryId); });
  m_framework.CreateDrapeEngine(make_ref(m_contextFactory), std::move(p));
  // A recording goes on after a restart.
  WatchRecording();
  m_framework.EnterForeground();
  m_contextFactory->WaitForInitialization(nullptr);
  UpdateWidgetLayout();
  UpdateVisibleViewport();

  m_updateTimer.start();
  m_compass->start();
}

void MapItem::Resize(int width, int height)
{
  m_framework.OnSize(width, height);
  if (!m_skin)
    return;

  m_skin->Resize(width, height);
  UpdateWidgetLayout();
  UpdateVisibleViewport();
}

void MapItem::setViewportBottomInset(qreal inset)
{
  if (inset == m_viewportBottomInset)
    return;
  m_viewportBottomInset = inset;
  emit viewportBottomInsetChanged();
  UpdateVisibleViewport();
}

void MapItem::UpdateVisibleViewport()
{
  // Ignored by the framework until the drape engine exists.
  double const bottom = std::max<double>(1.0, height() - m_viewportBottomInset);
  m_framework.SetVisibleViewport(m2::RectD(0, 0, width(), bottom));
}

void MapItem::setBottomWidgetsOffset(qreal offset)
{
  if (offset == m_bottomWidgetsOffset)
    return;
  m_bottomWidgetsOffset = offset;
  emit bottomWidgetsOffsetChanged();
  if (m_skin)
    UpdateWidgetLayout();
}

void MapItem::setTopWidgetsOffset(qreal offset)
{
  if (offset == m_topWidgetsOffset)
    return;
  m_topWidgetsOffset = offset;
  emit topWidgetsOffsetChanged();
  if (m_skin)
    UpdateWidgetLayout();
}

void MapItem::UpdateWidgetLayout()
{
  gui::TWidgetsLayoutInfo layout;
  m_skin->ForEach([&layout](gui::EWidget w, gui::Position const & pos) { layout[w] = pos.m_pixelPivot; });
  // Both are anchored to the bottom left corner; the ruler sits a little above the attribution.
  double const offset = m_bottomWidgetsOffset;
  layout[gui::WIDGET_RULER].y -= offset + 8 * m_visualScale;
  layout[gui::WIDGET_COPYRIGHT].y -= offset;
  // The Android margin_compass plus nav_frame_padding from the right and margin_compass_top below the top
  // controls; the offset already includes the padding.
  bool const landscape = width() > height();
  layout[gui::WIDGET_COMPASS] =
      m2::PointD(width() - (24 + 8) * m_visualScale, m_topWidgetsOffset + (landscape ? 20 : 26) * m_visualScale);
  m_framework.SetWidgetLayout(std::move(layout));
}

void MapItem::geometryChanged(QRectF const & newGeometry, QRectF const & oldGeometry)
{
  QQuickItem::geometryChanged(newGeometry, oldGeometry);
  if (newGeometry.size() == oldGeometry.size())
    return;

  if (m_contextFactory)
    Resize(static_cast<int>(newGeometry.width()), static_cast<int>(newGeometry.height()));
  else
    CreateEngine();
}

QSGNode * MapItem::updatePaintNode(QSGNode * oldNode, UpdatePaintNodeData *)
{
  if (!m_contextFactory || !m_contextFactory->AcquireFrame())
    return oldNode;

  GLuint const textureId = m_contextFactory->GetTextureHandle();
  QRectF const & texRect = m_contextFactory->GetTexRect();
  if (textureId == 0 || texRect.width() <= 0 || texRect.height() <= 0)
    return oldNode;

  auto * node = static_cast<MapTextureNode *>(oldNode);
  if (!node)
    node = new MapTextureNode();

  QSize const usedSize(static_cast<int>(width()), static_cast<int>(height()));
  QSize const textureSize(qRound(usedSize.width() / texRect.width()), qRound(usedSize.height() / texRect.height()));
  node->Update(window(), textureId, textureSize, usedSize);
  node->setRect(boundingRect());
  return node;
}

void MapItem::touchEvent(QTouchEvent * event)
{
  if (!m_contextFactory)
    return;

  if (event->type() == QEvent::TouchCancel)
  {
    CancelTouch();
    return;
  }

  auto const & points = event->touchPoints();
  if (points.isEmpty())
    return;

  df::TouchEvent touchEvent;
  // The fingers still down, to end the gesture with when it is cancelled.
  df::TouchEvent held;
  bool hasHeld = false;
  for (int i = 0; i < points.size() && i < 2; ++i)
  {
    df::Touch touch;
    touch.m_id = points[i].id();
    touch.m_location = m2::PointD(points[i].pos().x(), points[i].pos().y());
    if (i == 0)
      touchEvent.SetFirstTouch(touch);
    else
      touchEvent.SetSecondTouch(touch);
    if (points[i].state() == Qt::TouchPointReleased)
      continue;
    if (hasHeld)
      held.SetSecondTouch(touch);
    else
      held.SetFirstTouch(touch);
    hasHeld = true;
  }

  // The core expects the finger that changed, as from an Android MotionEvent.
  uint8_t maskedPointer = df::TouchEvent::INVALID_MASKED_POINTER;
  df::TouchEvent::ETouchType type = df::TouchEvent::TOUCH_MOVE;
  for (int i = 0; i < points.size() && i < 2; ++i)
  {
    if (points[i].state() == Qt::TouchPointPressed)
    {
      type = df::TouchEvent::TOUCH_DOWN;
      maskedPointer = static_cast<uint8_t>(i);
    }
    else if (points[i].state() == Qt::TouchPointReleased)
    {
      type = df::TouchEvent::TOUCH_UP;
      maskedPointer = static_cast<uint8_t>(i);
    }
  }
  touchEvent.SetTouchType(type);
  touchEvent.SetFirstMaskedPointer(maskedPointer);
  m_framework.TouchEvent(touchEvent);
  m_heldTouches = held;
  m_touchActive = hasHeld;
  event->accept();
}

void MapItem::CancelTouch()
{
  if (!m_contextFactory || !m_touchActive)
    return;
  m_touchActive = false;
  m_heldTouches.SetTouchType(df::TouchEvent::TOUCH_CANCEL);
  m_framework.TouchEvent(m_heldTouches);
}

// A Silica flickable or the page stack can take the grab in the middle of a gesture.
void MapItem::touchUngrabEvent()
{
  CancelTouch();
}

void MapItem::mouseUngrabEvent()
{
  CancelTouch();
}

void MapItem::SendMouseTouch(QMouseEvent * event, df::TouchEvent::ETouchType touchType)
{
  if (!m_contextFactory)
    return;

  df::Touch touch;
  touch.m_id = 0;
  touch.m_location = m2::PointD(event->localPos().x(), event->localPos().y());

  df::TouchEvent touchEvent;
  touchEvent.SetTouchType(touchType);
  touchEvent.SetFirstTouch(touch);
  m_framework.TouchEvent(touchEvent);
  m_heldTouches = touchEvent;
  m_touchActive = touchType != df::TouchEvent::TOUCH_UP;
  event->accept();
}

void MapItem::mousePressEvent(QMouseEvent * event)
{
  SendMouseTouch(event, df::TouchEvent::TOUCH_DOWN);
}

void MapItem::mouseMoveEvent(QMouseEvent * event)
{
  SendMouseTouch(event, df::TouchEvent::TOUCH_MOVE);
}

void MapItem::mouseReleaseEvent(QMouseEvent * event)
{
  SendMouseTouch(event, df::TouchEvent::TOUCH_UP);
}
}  // namespace sailfish
