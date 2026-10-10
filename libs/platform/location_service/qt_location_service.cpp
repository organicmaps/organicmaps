#include "platform/location_service/qt_location_service.hpp"

#include "base/logging.hpp"

#include <QGeoPositionInfoSource>

namespace
{
location::GpsInfo GpsInfoFromQGeoPositionInfo(QGeoPositionInfo const & i, location::TLocationSource source)
{
  location::GpsInfo info;
  info.m_source = source;

  auto const coord = i.coordinate();
  info.m_latitude = coord.latitude();
  info.m_longitude = coord.longitude();
  if (coord.type() == QGeoCoordinate::Coordinate3D)
    info.m_altitude = coord.altitude();

  info.m_timestamp = static_cast<double>(i.timestamp().toMSecsSinceEpoch() / 1000);

  if (i.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
    info.m_horizontalAccuracy = i.attribute(QGeoPositionInfo::HorizontalAccuracy);

  if (i.hasAttribute(QGeoPositionInfo::VerticalAccuracy))
    info.m_verticalAccuracy = i.attribute(QGeoPositionInfo::VerticalAccuracy);

  if (i.hasAttribute(QGeoPositionInfo::Direction))
    info.m_bearing = i.attribute(QGeoPositionInfo::Direction);

  if (i.hasAttribute(QGeoPositionInfo::GroundSpeed))
    info.m_speed = i.attribute(QGeoPositionInfo::GroundSpeed);

  return info;
}

static location::TLocationError TLocationErrorFromQGeoPositionInfoError(QGeoPositionInfoSource::Error error)
{
  location::TLocationError result = location::TLocationError::ENotSupported;
  switch (error)
  {
  case QGeoPositionInfoSource::AccessError: result = location::TLocationError::EDenied; break;
  case QGeoPositionInfoSource::ClosedError: result = location::TLocationError::EGPSIsOff; break;
  case QGeoPositionInfoSource::NoError: result = location::TLocationError::ENoError; break;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  case QGeoPositionInfoSource::UpdateTimeoutError: result = location::TLocationError::ETimeout; break;
#endif
  case QGeoPositionInfoSource::UnknownSourceError: result = location::TLocationError::EUnknown; break;
  default: break;
  }
  return result;
}

location::TLocationSource QStringToTLocationSource(QString const & sourceName)
{
  // An undefined source makes GpsInfo invalid, which drops every fix.
  if ("geoclue2" == sourceName || "geoclue" == sourceName)
    return location::TLocationSource::EGeoClue2;

  return location::TLocationSource::EUndefined;
}
}  // namespace

QtLocationService::QtLocationService(location::LocationObserver & observer, std::string const & sourceName)
  : LocationService(observer)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  QVariantMap params;
  params["desktopId"] = "app.organicmaps.desktop";
  m_positionSource = QGeoPositionInfoSource::createSource(QString::fromStdString(sourceName), params, this);
#else
  m_positionSource = QGeoPositionInfoSource::createSource(QString::fromStdString(sourceName), this);
#endif

  if (!m_positionSource)
  {
    LOG(LWARNING, ("Failed to acquire QGeoPositionInfoSource from ", sourceName));
    return;
  }

  if (!connect(m_positionSource, &QGeoPositionInfoSource::positionUpdated, this, &QtLocationService::OnLocationUpdate))
  {
    LOG(LERROR, ("Failed to connect the signal:", "positionUpdated"));
    return;
  }

  LOG(LDEBUG, ("Signal successfully connected:", "positionUpdated"));

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  auto const errorSignal = &QGeoPositionInfoSource::errorOccurred;
#else
  auto const errorSignal =
      static_cast<void (QGeoPositionInfoSource::*)(QGeoPositionInfoSource::Error)>(&QGeoPositionInfoSource::error);
#endif
  if (!connect(m_positionSource, errorSignal, this, &QtLocationService::OnErrorOccurred))
  {
    LOG(LERROR, ("Failed to connect the signal:", "errorOccurred"));
    return;
  }
  LOG(LDEBUG, ("Signal successfully connected:", "errorOccurred"));

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  if (!connect(m_positionSource, &QGeoPositionInfoSource::supportedPositioningMethodsChanged, this,
               &QtLocationService::OnSupportedPositioningMethodsChanged))
  {
    LOG(LERROR, ("Failed to connect the signal:", "supportedPositioningMethodsChanged"));
    return;
  }
  LOG(LDEBUG, ("Signal successfully connected:", "supportedPositioningMethodsChanged"));
#endif

  m_positionSource->setUpdateInterval(1000);
  m_positionSource->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
}

void QtLocationService::OnLocationUpdate(QGeoPositionInfo const & info)
{
  if (!info.isValid())
  {
    LOG(LWARNING,
        ("Location update with Invalid timestamp or coordinates from:", m_positionSource->sourceName().toStdString()));
    return;
  }
  auto const & coordinate = info.coordinate();
  LOG(LDEBUG, ("Location updated with valid coordinates:", coordinate.longitude(), coordinate.latitude()));
  m_observer.OnLocationUpdated(
      GpsInfoFromQGeoPositionInfo(info, QStringToTLocationSource(m_positionSource->sourceName())));
  if (!m_clientIsActive)
  {
    m_clientIsActive = true;
    m_positionSource->startUpdates();
  }
}

void QtLocationService::OnErrorOccurred(QGeoPositionInfoSource::Error positioningError)
{
  LOG(LWARNING, ("Location error occured QGeoPositionInfoSource::Error code:", positioningError));
  m_clientIsActive = false;
  m_observer.OnLocationError(TLocationErrorFromQGeoPositionInfoError(positioningError));
}

void QtLocationService::OnSupportedPositioningMethodsChanged()
{
  auto positioningMethods = m_positionSource->supportedPositioningMethods();
  LOG(LDEBUG, ("Supported Positioning Method changed for:", m_positionSource->sourceName().toStdString(),
               "to:", positioningMethods));
  if (positioningMethods == QGeoPositionInfoSource::NoPositioningMethods)
  {
    m_clientIsActive = false;
    m_observer.OnLocationError(location::TLocationError::EGPSIsOff);
  }
}

void QtLocationService::Start()
{
  if (m_positionSource)
  {
    LOG(LDEBUG, ("Starting Updates from:", m_positionSource->sourceName().toStdString()));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Request the first update with a timeout to 30 minutes which is needed on devices that don't make use of `A-GNSS`
    // and can't get a lock within Qt's default `UPDATE_TIMEOUT_COLDSTART` (currently 2 minutes).
    m_positionSource->requestUpdate(1800000);
#else
    // Qt 5 has no geoclue2 Active state to wait for, so subscribe right away.
    m_positionSource->startUpdates();
    m_clientIsActive = true;
#endif
  }
}

void QtLocationService::Stop()
{
  if (m_positionSource && m_clientIsActive)
  {
    LOG(LDEBUG, ("Stopping Updates from:", m_positionSource->sourceName().toStdString()));
    m_positionSource->stopUpdates();
  }
}

std::unique_ptr<location::LocationService> CreateQtLocationService(location::LocationObserver & observer,
                                                                   std::string const & sourceName)
{
  return std::make_unique<QtLocationService>(observer, sourceName);
}
