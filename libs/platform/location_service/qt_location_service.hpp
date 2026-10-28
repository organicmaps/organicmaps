#pragma once

#include "platform/location_service/location_service.hpp"
#include "platform/location_service/position_filter.hpp"

#include <QGeoPositionInfoSource>
#include <QTimer>

class QtLocationService
  : public QObject
  , public location::LocationService
{
  Q_OBJECT
  QGeoPositionInfoSource * m_positionSource;
  QTimer m_firstFixTimeout{this};
  location::PositionFilter m_filter;
  // A stopped provider can still deliver cached or queued signals.
  bool m_started = false;

public:
  explicit QtLocationService(location::LocationObserver &, std::string const &);
  // Takes ownership of the provider.
  QtLocationService(location::LocationObserver &, QGeoPositionInfoSource *);
  ~QtLocationService() override;
  void Start() override;
  void Stop() override;

public slots:
  void OnLocationUpdate(QGeoPositionInfo const &);
  void OnErrorOccurred(QGeoPositionInfoSource::Error);
  void OnSupportedPositioningMethodsChanged();
};
