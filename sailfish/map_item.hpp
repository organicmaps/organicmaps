#pragma once

#include "qt/qt_common/qtoglcontextfactory.hpp"

#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "platform/location_service/location_service.hpp"

#include <QQuickItem>
#include <QVariantMap>

#include <memory>

class Framework;
class QCompass;

namespace sailfish
{
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
  // Width hidden by the camera cutout on each side, which the map is drawn under in landscape.
  Q_PROPERTY(qreal leftInset READ leftInset WRITE setLeftInset NOTIFY horizontalInsetsChanged)
  Q_PROPERTY(qreal rightInset READ rightInset WRITE setRightInset NOTIFY horizontalInsetsChanged)
  // {address, coordinates, altitude, speed} for the app cover; empty without a position.
  Q_PROPERTY(QVariantMap positionInfo READ positionInfo NOTIFY positionInfoChanged)
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
  // Empty without a position.
  Q_INVOKABLE QString myPositionShareText() const;

  // Hiking and cycling routes need newer maps here.
  Q_INVOKABLE bool needUpdateForRoutes() const;
  // Contour lines are on but not shown at this zoom.
  Q_INVOKABLE bool isolinesNeedZoom() const;

  int myPositionMode() const { return m_myPositionMode; }
  qreal leftInset() const { return m_leftInset; }
  void setLeftInset(qreal inset);
  qreal rightInset() const { return m_rightInset; }
  void setRightInset(qreal inset);
  QVariantMap positionInfo() const { return m_positionInfo; }
  qreal bottomWidgetsOffset() const { return m_bottomWidgetsOffset; }
  void setBottomWidgetsOffset(qreal offset);
  qreal topWidgetsOffset() const { return m_topWidgetsOffset; }
  void setTopWidgetsOffset(qreal offset);
  int enabledLayers() const;
  int newLayers() const;

signals:
  void engineCreated();
  void myPositionModeChanged();
  void layersChanged();
  void bottomWidgetsOffsetChanged();
  void topWidgetsOffsetChanged();
  void positionInfoChanged();
  void notice(QString const & message);
  // Contour lines need newer maps here.
  void isolinesNeedMaps();
  void horizontalInsetsChanged();

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
  // Location and the compass run in the foreground while the position is on. Restart also starts a running service
  // again.
  void UpdateLocationService(bool restart = false);
  void OnCompassReading();
  void UpdatePositionInfo();

  // location::LocationObserver
  void OnLocationError(location::TLocationError errorCode) override;
  void OnLocationUpdated(location::GpsInfo const & info) override;

  Framework & m_framework;
  std::unique_ptr<qt::common::QtOGLContextFactory> m_contextFactory;
  std::unique_ptr<gui::Skin> m_skin;
  std::unique_ptr<location::LocationService> m_locationService;
  QCompass * m_compass = nullptr;
  location::EMyPositionMode m_myPositionMode = location::PendingPosition;
  df::TouchEvent m_heldTouches;
  bool m_touchActive = false;
  double m_visualScale = 1.0;
  qreal m_bottomWidgetsOffset = 0;
  qreal m_topWidgetsOffset = 0;
  qreal m_leftInset = 0;
  qreal m_rightInset = 0;
  bool m_inBackground = false;
  bool m_locationActive = false;
  bool m_locationErrorShown = false;
  QVariantMap m_positionInfo;
  // The last fix for the cover, and when positionInfo was updated.
  bool m_hasAltitude = false;
  double m_altitude = 0;
  double m_speed = -1;
  qint64 m_positionInfoMs = 0;
  bool m_calibrationShown = false;
};
}  // namespace sailfish
