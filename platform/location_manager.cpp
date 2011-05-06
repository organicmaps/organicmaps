#include "location.hpp"

#include "../std/target_os.hpp"
#include "../std/vector.hpp"
#include "../std/bind.hpp"

double ApproxDistanceSquareInMetres(double lat1, double lon1, double lat2, double lon2)
{
  double const m1 = (lat1 - lat2) / 111111.;
  double const m2 = (lon1 - lon2) / 111111.;
  return m1 * m1 + m2 * m2;
}

/// Chooses most accurate data from different position services
class PositionFilter
{
  location::GpsInfo * m_prevLocation;
public:
  PositionFilter() : m_prevLocation(NULL) {}
  ~PositionFilter() { delete m_prevLocation; }
  /// @return true if location should be sent to observers
  bool Passes(location::GpsInfo const & newLocation)
  {
    if (time(NULL) - newLocation.m_timestamp > location::POSITION_TIMEOUT_SECONDS)
      return false;

    bool passes = true;
    if (m_prevLocation)
    {
      if (newLocation.m_timestamp < m_prevLocation->m_timestamp)
        passes = false;
      else if (newLocation.m_source != m_prevLocation->m_source
               && newLocation.m_horizontalAccuracy > m_prevLocation->m_horizontalAccuracy
               && ApproxDistanceSquareInMetres(newLocation.m_latitude,
                                               newLocation.m_longitude,
                                               m_prevLocation->m_latitude,
                                               m_prevLocation->m_longitude)
               > newLocation.m_horizontalAccuracy * newLocation.m_horizontalAccuracy)
        passes = false;
    }
    else
      m_prevLocation = new location::GpsInfo(newLocation);
    return true;
  }
};

namespace location
{
  class LocationManager : public LocationService
  {
    vector<LocationService *> m_services;
    PositionFilter m_filter;

    void OnGpsUpdate(GpsInfo const & info)
    {
      if (m_filter.Passes(info))
        NotifyGpsObserver(info);
    }

    void OnCompassUpdate(CompassInfo const & info)
    {
      NotifyCompassObserver(info);
    }

  public:
    LocationManager()
    {
      LocationService * service;

#if defined(OMIM_OS_IPHONE) || defined(OMIM_OS_MAC)
      service = CreateAppleLocationService();
      service->SetGpsObserver(bind(&LocationManager::OnGpsUpdate, this, _1));
      service->SetCompassObserver(bind(&LocationManager::OnCompassUpdate, this, _1));
      m_services.push_back(service);
#endif

#if defined(OMIM_OS_WINDOWS) || defined(OMIM_OS_MAC)
      service = CreateWiFiLocationService();
      service->SetGpsObserver(bind(&LocationManager::OnGpsUpdate, this, _1));
      m_services.push_back(service);
#endif
    }

    virtual ~LocationManager()
    {
      for (size_t i = 0; i < m_services.size(); ++i)
        delete m_services[i];
    }

    virtual void StartUpdate(bool useAccurateMode)
    {
      for (size_t i = 0; i < m_services.size(); ++i)
        m_services[i]->StartUpdate(useAccurateMode);
    }

    virtual void StopUpdate()
    {
      for (size_t i = 0; i < m_services.size(); ++i)
        m_services[i]->StopUpdate();
    }
  };
}

location::LocationService & GetLocationManager()
{
  static location::LocationManager mgr;
  return mgr;
}
