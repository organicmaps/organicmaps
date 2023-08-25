#include "platform/qt_location_service.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include "std/target_os.hpp"

#include <QGeoPositionInfoSource>

namespace
{
static location::GpsInfo gpsInfoFromQGeoPositionInfo(QGeoPositionInfo const & i, location::TLocationSource source)
{
  location::GpsInfo info;
  info.m_source = source;

  info.m_latitude = i.coordinate().latitude();
  info.m_longitude = i.coordinate().longitude();
  info.m_timestamp = static_cast<double>(i.timestamp().toSecsSinceEpoch());

  if (i.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
    info.m_horizontalAccuracy = static_cast<double>(i.attribute(QGeoPositionInfo::HorizontalAccuracy));

  if (i.hasAttribute(QGeoPositionInfo::VerticalAccuracy))
    info.m_verticalAccuracy = static_cast<double>(i.attribute(QGeoPositionInfo::VerticalAccuracy));

  if (i.hasAttribute(QGeoPositionInfo::Direction))
    info.m_bearing = static_cast<double>(i.attribute(QGeoPositionInfo::Direction));

  if (i.hasAttribute(QGeoPositionInfo::GroundSpeed))
    info.m_speedMpS = static_cast<double>(i.attribute(QGeoPositionInfo::GroundSpeed));

  return info;
}

static location::TLocationError tLocationErrorFromQGeoPositionInfoError(QGeoPositionInfoSource::Error error)
{
  location::TLocationError result = location::TLocationError::ENotSupported;
  switch(error)
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

static location::TLocationSource qStringToTLocationSource(QString const & sourceName)
{
  if ("geoclue2" == sourceName)
    return location::TLocationSource::EGeoClue2;

  return location::TLocationSource::EUndefined;
}
};


QtLocationService::QtLocationService(location::LocationObserver & observer, std::string const & sourceName) : LocationService(observer)
{
  QVariantMap params;
  params["desktopId"] = "OrganicMaps";
  m_positionSource = QGeoPositionInfoSource::createSource(QString::fromStdString(sourceName), params, this);

  if (!m_positionSource)
  {
    LOG(LWARNING, ("Failed to acquire QGeoPositionInfoSource from ", sourceName));
    return;
  }

  if (!connect(m_positionSource, &QGeoPositionInfoSource::positionUpdated,
               this, &QtLocationService::OnLocationUpdate))
  {
    LOG(LERROR, ("Failed to connect the signal:", "positionUpdated"));
    return;
  }

  LOG(LDEBUG, ("Signal successfully connected:", "positionUpdated"));

  if (!connect(m_positionSource, &QGeoPositionInfoSource::errorOccurred,
               this, &QtLocationService::OnErrorOccurred))
  {
    LOG(LERROR, ("Failed to connect the signal:", "errorOccurred"));
    return;
  }
  LOG(LDEBUG, ("Signal successfully connected:", "errorOccurred"));


  if (!connect(m_positionSource, &QGeoPositionInfoSource::supportedPositioningMethodsChanged,
               this, &QtLocationService::OnSupportedPositioningMethodsChanged))
  {
    LOG(LERROR, ("Failed to connect the signal:", "supportedPositioningMethodsChanged"));
    return;
  }
  LOG(LDEBUG, ("Signal successfully connected:", "supportedPositioningMethodsChanged"));

  m_positionSource->setUpdateInterval(1000);
  m_positionSource->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
}

void QtLocationService::OnLocationUpdate(QGeoPositionInfo const & info)
{
  if (!info.isValid())
  {
    LOG(LWARNING, ("Location update with Invalid timestamp or coordinates from:", m_positionSource->sourceName().toStdString()));
    return;
  }
  auto const & coordinate = info.coordinate();
  LOG(LDEBUG, ("Location updated with valid coordinates:", coordinate.longitude(), coordinate.latitude()));
  m_observer.OnLocationUpdated(gpsInfoFromQGeoPositionInfo(info, qStringToTLocationSource(m_positionSource->sourceName())));
}

void QtLocationService::OnErrorOccurred(QGeoPositionInfoSource::Error positioningError)
{
  LOG(LWARNING, ("Location error occured QGeoPositionInfoSource::Error code:", positioningError));
  m_observer.OnLocationError(tLocationErrorFromQGeoPositionInfoError(positioningError));
}

void QtLocationService::OnSupportedPositioningMethodsChanged()
{
  LOG(LWARNING, ("Supported Positioning Method changed:", m_positionSource->sourceName().toStdString()));
  // In certain cases propagating GPSIsOff would make sense,
  // but this signal can also mean GPSISOn ... or something entirely different.
  // m_observer.OnLocationError(location::TLocationError::EGPSIsOff);
}

void QtLocationService::Start()
{
  if (!m_positionSource)
  {
    LOG(LWARNING, ("No supported source is available for starting the positioning with:", m_positionSource->sourceName().toStdString()));
  }
  else
  {
    LOG(LDEBUG, ("Starting Updates from:", m_positionSource->sourceName().toStdString()));
    m_positionSource->startUpdates();
    QGeoPositionInfo info = m_positionSource->lastKnownPosition();
    m_positionSource->requestUpdate();
  }
}

void QtLocationService::Stop()
{
  if (!m_positionSource)
  {
    LOG(LWARNING, ("No supported source is available for stopping the positioning with:", m_positionSource->sourceName().toStdString()));
  }
  else
  {
    LOG(LDEBUG, ("Stopping Updates from:", m_positionSource->sourceName().toStdString()));
    m_positionSource->stopUpdates();
  }
}

std::unique_ptr<location::LocationService> CreateQtLocationService(location::LocationObserver & observer, std::string const & sourceName)
{
  return std::make_unique<QtLocationService>(observer, sourceName);
}
