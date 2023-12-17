#include "platform/location_service.hpp"

#include <QGeoPositionInfoSource>

class QtLocationService : public QObject, public location::LocationService
{
  Q_OBJECT
  QGeoPositionInfoSource *m_positionSource;

public:
  explicit QtLocationService(location::LocationObserver &, std::string const &);
  virtual ~QtLocationService() {};
  virtual void Start();
  virtual void Stop();

public slots:
  void OnLocationUpdate(QGeoPositionInfo const &);
  void OnErrorOccurred(QGeoPositionInfoSource::Error);
  void OnSupportedPositioningMethodsChanged();
};
