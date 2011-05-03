#include "location.hpp"

#include "../std/target_os.hpp"
#include "../std/vector.hpp"
#include "../std/bind.hpp"

/// Chooses most accurate data from different position services
class PositionFilter
{
public:
  GpsInfo const & MostNewAndAccuratePosition()
  {
  }
};

namespace location
{
  class LocationManager : public LocationService
  {
    vector<LocationService *> m_services;

    void OnGpsUpdate(GpsInfo const & info)
    {
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
