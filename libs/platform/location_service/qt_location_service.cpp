#include "platform/location_service/qt_location_service.hpp"

#include "base/assert.hpp"
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

  info.m_timestamp = static_cast<double>(i.timestamp().toSecsSinceEpoch());

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
  case QGeoPositionInfoSource::UpdateTimeoutError: result = location::TLocationError::ETimeout; break;
  case QGeoPositionInfoSource::UnknownSourceError: result = location::TLocationError::EUnknown; break;
  default: break;
  }
  return result;
}

location::TLocationSource QStringToTLocationSource(QString const & sourceName)
{
  if ("geoclue2" == sourceName)
    return location::TLocationSource::EGeoClue2;

  return location::TLocationSource::EUndefined;
}

QGeoPositionInfoSource * CreatePositionSource(std::string const & sourceName)
{
  QVariantMap params;
  params["desktopId"] = "app.organicmaps.desktop";
  auto * source = QGeoPositionInfoSource::createSource(QString::fromStdString(sourceName), params, nullptr);
  if (!source)
    LOG(LWARNING, ("Failed to acquire QGeoPositionInfoSource from", sourceName));
  return source;
}
}  // namespace

QtLocationService::QtLocationService(location::LocationObserver & observer, std::string const & sourceName)
  : QtLocationService(observer, CreatePositionSource(sourceName))
{}

QtLocationService::QtLocationService(location::LocationObserver & observer, QGeoPositionInfoSource * source)
  : LocationService(observer)
  , m_positionSource(source)
{
  if (!m_positionSource)
    return;

  m_positionSource->setParent(this);
  CHECK(connect(m_positionSource, &QGeoPositionInfoSource::positionUpdated, this, &QtLocationService::OnLocationUpdate),
        ());
  CHECK(connect(m_positionSource, &QGeoPositionInfoSource::errorOccurred, this, &QtLocationService::OnErrorOccurred),
        ());
  CHECK(connect(m_positionSource, &QGeoPositionInfoSource::supportedPositioningMethodsChanged, this,
                &QtLocationService::OnSupportedPositioningMethodsChanged),
        ());

  // Allow receivers without assisted GPS up to 30 minutes to acquire a usable fix.
  // Keep the deadline in this service so Stop() cancels acquisition and streaming together.
  m_firstFixTimeout.setSingleShot(true);
  m_firstFixTimeout.setInterval(30 * 60 * 1000);
  CHECK(connect(&m_firstFixTimeout, &QTimer::timeout, this,
                [this]
  {
    if (!m_started)
      return;
    Stop();
    m_observer.OnLocationError(location::ETimeout);
  }),
        ());

  m_positionSource->setUpdateInterval(1000);
  m_positionSource->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
}

QtLocationService::~QtLocationService()
{
  Stop();
}

void QtLocationService::OnLocationUpdate(QGeoPositionInfo const & info)
{
  if (!m_started)
    return;
  if (!info.isValid())
  {
    LOG(LWARNING,
        ("Location update with Invalid timestamp or coordinates from:", m_positionSource->sourceName().toStdString()));
    return;
  }
  auto const gpsInfo = GpsInfoFromQGeoPositionInfo(info, QStringToTLocationSource(m_positionSource->sourceName()));
  if (!m_filter.Passes(gpsInfo))
    return;

  auto const & coordinate = info.coordinate();
  LOG(LDEBUG, ("Location updated with valid coordinates:", coordinate.longitude(), coordinate.latitude()));
  m_firstFixTimeout.stop();
  m_observer.OnLocationUpdated(gpsInfo);
}

void QtLocationService::OnErrorOccurred(QGeoPositionInfoSource::Error positioningError)
{
  if (!m_started || positioningError == QGeoPositionInfoSource::NoError)
    return;
  LOG(LWARNING, ("Location error occurred QGeoPositionInfoSource::Error code:", positioningError));
  m_firstFixTimeout.stop();
  m_observer.OnLocationError(TLocationErrorFromQGeoPositionInfoError(positioningError));
}

void QtLocationService::OnSupportedPositioningMethodsChanged()
{
  if (!m_started)
    return;
  auto positioningMethods = m_positionSource->supportedPositioningMethods();
  LOG(LDEBUG, ("Supported Positioning Method changed for:", m_positionSource->sourceName().toStdString(),
               "to:", positioningMethods));
  if (positioningMethods == QGeoPositionInfoSource::NoPositioningMethods)
    OnErrorOccurred(QGeoPositionInfoSource::ClosedError);
}

void QtLocationService::Start()
{
  if (!m_positionSource || m_started)
    return;

  m_started = true;
  LOG(LDEBUG, ("Starting Updates from:", m_positionSource->sourceName().toStdString()));
  m_firstFixTimeout.start();
  m_positionSource->startUpdates();
}

void QtLocationService::Stop()
{
  m_firstFixTimeout.stop();
  if (m_started)
  {
    m_started = false;
    LOG(LDEBUG, ("Stopping Updates from:", m_positionSource->sourceName().toStdString()));
    m_positionSource->stopUpdates();
  }
}

std::unique_ptr<location::LocationService> CreateQtLocationService(location::LocationObserver & observer,
                                                                   std::string const & sourceName)
{
  return std::make_unique<QtLocationService>(observer, sourceName);
}
