#pragma once

#include "qt/qt_common/qtoglcontextfactory.hpp"

#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "platform/location_service/location_service.hpp"

#include <QQuickItem>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <memory>
#include <string>

class Framework;
class QCompass;

namespace sailfish
{
class PlacePage;
class Routing;

// Hosts the drape engine inside Qt Quick. Drape renders into offscreen framebuffers in
// contexts shared through QOpenGLContext::globalShareContext(); the item shows the last
// presented frame as a scene graph texture.
class MapItem
  : public QQuickItem
  , public location::LocationObserver
{
  Q_OBJECT
  Q_PROPERTY(int myPositionMode READ myPositionMode NOTIFY myPositionModeChanged)
  // Bit mask of Layer values.
  Q_PROPERTY(int enabledLayers READ enabledLayers NOTIFY layersChanged)
  // Bit mask of the layers marked as new until first tapped.
  Q_PROPERTY(int newLayers READ newLayers NOTIFY layersChanged)
  Q_PROPERTY(sailfish::PlacePage * placePage READ placePage CONSTANT)
  Q_PROPERTY(sailfish::Routing * routing READ routing CONSTANT)
  // Height covered by panels at the bottom; routes and searches are fitted into the map above it.
  Q_PROPERTY(
      qreal viewportBottomInset READ viewportBottomInset WRITE setViewportBottomInset NOTIFY viewportBottomInsetChanged)
  Q_PROPERTY(bool trackRecording READ trackRecording NOTIFY trackRecordingChanged)
  Q_PROPERTY(QString recordingSummary READ recordingSummary NOTIFY recordingStatsChanged)
  // ElevationChart() data, empty while nothing is recorded.
  Q_PROPERTY(QVariantMap recordingElevation READ recordingElevation NOTIFY recordingStatsChanged)
  // {address, coordinates, altitude, speed} for the app cover; empty without a position.
  Q_PROPERTY(QVariantMap positionInfo READ positionInfo NOTIFY positionInfoChanged)
  Q_PROPERTY(bool locationLost READ locationLost NOTIFY locationLostChanged)
  // Taps don't select places meanwhile.
  Q_PROPERTY(bool choosingPosition READ choosingPosition NOTIFY choosingPositionChanged)
  // The region in the middle of the map while its map isn't downloaded: {countryId, name, size, status (a
  // CountriesModel::Status), progress (0..1)}; empty otherwise.
  Q_PROPERTY(QVariantMap currentCountry READ currentCountry NOTIFY currentCountryChanged)
  // The scale line and attribution stay above it.
  Q_PROPERTY(
      qreal bottomWidgetsOffset READ bottomWidgetsOffset WRITE setBottomWidgetsOffset NOTIFY bottomWidgetsOffsetChanged)
  // Height of the map controls along the top right edge; the compass stays below them.
  Q_PROPERTY(qreal topWidgetsOffset READ topWidgetsOffset WRITE setTopWidgetsOffset NOTIFY topWidgetsOffsetChanged)

public:
  enum Layer
  {
    Outdoors,
    Isolines,
    Hiking,
    Cycling,
    Subway,
    // Offered once a tile server is set in the settings.
    Satellite
  };
  Q_ENUM(Layer)

  // Mirrors location::EMyPositionMode for QML.
  enum MyPositionMode
  {
    PendingPosition,
    NotFollowNoPosition,
    NotFollow,
    Follow,
    FollowAndRotate
  };
  Q_ENUM(MyPositionMode)

  explicit MapItem(QQuickItem * parent = nullptr);
  ~MapItem() override;

  Q_INVOKABLE void zoomIn();
  Q_INVOKABLE void zoomOut();
  Q_INVOKABLE void switchMyPositionMode();

  Q_INVOKABLE void setLayerEnabled(int layer, bool enabled);
  // Location keeps running in the background while recording.
  Q_INVOKABLE void startTrackRecording();
  // Discards the recorded track.
  Q_INVOKABLE void stopTrackRecording();
  // Saves under the default name; false when there was nothing to save.
  Q_INVOKABLE bool saveAndStopTrackRecording();
  // Empty without a position.
  Q_INVOKABLE QString myPositionShareText() const;

  // Starts at the selected place, if any. For a business the cross stays inside the selected building.
  Q_INVOKABLE void startChoosingPosition(bool business = false);
  Q_INVOKABLE void stopChoosingPosition();
  // Ends choosing and returns [lat, lon] of the cross; empty outside downloaded maps when they are required.
  Q_INVOKABLE QVariantList confirmChosenPosition(bool requireMaps = true);

  // The map in the middle is too old to edit, see MissingMapInfo(); empty otherwise.
  Q_INVOKABLE QVariantMap mapToUpdateForEditing() const;
  Q_INVOKABLE void downloadMap(QString const & countryId);
  Q_INVOKABLE void cancelMap(QString const & countryId);
  // Hiking and cycling routes need newer maps here.
  Q_INVOKABLE bool needUpdateForRoutes() const;
  // Contour lines are on but not shown at this zoom.
  Q_INVOKABLE bool isolinesNeedZoom() const;

  int myPositionMode() const { return m_myPositionMode; }
  PlacePage * placePage() const { return m_placePage.get(); }
  Routing * routing() const { return m_routing.get(); }
  qreal viewportBottomInset() const { return m_viewportBottomInset; }
  void setViewportBottomInset(qreal inset);
  QString recordingSummary() const { return m_recordingSummary; }
  QVariantMap positionInfo() const { return m_positionInfo; }
  bool locationLost() const { return m_locationLost; }
  QVariantMap recordingElevation() const { return m_recordingElevation; }
  bool trackRecording() const;
  bool choosingPosition() const { return m_choosingPosition; }
  qreal bottomWidgetsOffset() const { return m_bottomWidgetsOffset; }
  void setBottomWidgetsOffset(qreal offset);
  qreal topWidgetsOffset() const { return m_topWidgetsOffset; }
  void setTopWidgetsOffset(qreal offset);
  int enabledLayers() const;
  int newLayers() const;
  QVariantMap currentCountry() const { return m_currentCountry; }

signals:
  void myPositionModeChanged();
  void layersChanged();
  void bottomWidgetsOffsetChanged();
  void topWidgetsOffsetChanged();
  void trackRecordingChanged();
  void recordingStatsChanged();
  void positionInfoChanged();
  void locationLostChanged();
  void notice(QString const & message);
  // Contour lines need newer maps here.
  void isolinesNeedMaps();
  void choosingPositionChanged();
  void viewportBottomInsetChanged();
  void currentCountryChanged();

protected:
  QSGNode * updatePaintNode(QSGNode * oldNode, UpdatePaintNodeData *) override;
  void geometryChanged(QRectF const & newGeometry, QRectF const & oldGeometry) override;
  void touchEvent(QTouchEvent * event) override;
  void mousePressEvent(QMouseEvent * event) override;
  void mouseMoveEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent * event) override;
  void touchUngrabEvent() override;
  void mouseUngrabEvent() override;

private:
  void OnWindowChanged(QQuickWindow * window);
  void OnApplicationStateChanged(Qt::ApplicationState state);
  void CreateEngine();
  void Resize(int width, int height);
  void UpdateWidgetLayout();
  void UpdateVisibleViewport();
  void SendMouseTouch(QMouseEvent * event, df::TouchEvent::ETouchType touchType);
  // Drape needs the touches of a gesture to end it: Qt sends TouchCancel without points.
  void CancelTouch();
  void OnMyPositionModeChanged(location::EMyPositionMode mode);
  void OnCompassReading();
  // Auto-downloads the map of the region the user is in.
  void OnCurrentCountryChanged(std::string const & countryId);
  // Statistics follow a running recording.
  void WatchRecording();
  void UpdatePositionInfo();
  void UpdateCurrentCountry();

  // location::LocationObserver
  void OnLocationError(location::TLocationError errorCode) override;
  void OnLocationUpdated(location::GpsInfo const & info) override;

  Framework & m_framework;
  std::unique_ptr<qt::common::QtOGLContextFactory> m_contextFactory;
  std::unique_ptr<gui::Skin> m_skin;
  std::unique_ptr<location::LocationService> m_locationService;
  std::unique_ptr<PlacePage> m_placePage;
  std::unique_ptr<Routing> m_routing;
  bool m_choosingPosition = false;
  QCompass * m_compass = nullptr;
  location::EMyPositionMode m_myPositionMode = location::PendingPosition;
  QTimer m_updateTimer;
  df::TouchEvent m_heldTouches;
  bool m_touchActive = false;
  double m_visualScale = 1.0;
  qreal m_bottomWidgetsOffset = 0;
  qreal m_topWidgetsOffset = 0;
  qreal m_viewportBottomInset = 0;
  bool m_inBackground = false;
  bool m_locationErrorShown = false;
  bool m_locationLost = false;
  bool m_hadPosition = false;
  QVariantMap m_positionInfo;
  // The last fix for the cover, and when positionInfo was updated.
  bool m_hasAltitude = false;
  double m_altitude = 0;
  double m_speed = -1;
  qint64 m_positionInfoMs = 0;
  bool m_calibrationShown = false;
  QString m_recordingSummary;
  QVariantMap m_recordingElevation;
  std::string m_currentCountryId;
  QVariantMap m_currentCountry;
  int m_storageSlot = 0;
};
}  // namespace sailfish
