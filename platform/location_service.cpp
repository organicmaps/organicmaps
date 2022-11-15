#include "platform/location_service.hpp"

#include "std/target_os.hpp"

#include <ctime>
#include <optional>
#include <vector>

extern "C" location::LocationService * CreateAppleLocationService(location::LocationObserver &);

namespace location
{
static double ApproxDistanceSquareInMeters(double lat1, double lon1, double lat2, double lon2)
{
  double const m1 = (lat1 - lat2) / 111111.;
  double const m2 = (lon1 - lon2) / 111111.;
  return m1 * m1 + m2 * m2;
}

/// Chooses most accurate data from different position services
class PositionFilter
{
  std::optional<location::GpsInfo> m_prevLocation;
public:
  /// @return true if location should be sent to observers
  bool Passes(location::GpsInfo const & newLocation)
  {
    if (std::time(nullptr) - newLocation.m_timestamp > 300.0)
      return false;

    bool passes = true;
    if (m_prevLocation)
    {
      if (newLocation.m_timestamp < m_prevLocation->m_timestamp)
        passes = false;
      else if (newLocation.m_source != m_prevLocation->m_source
               && newLocation.m_horizontalAccuracy > m_prevLocation->m_horizontalAccuracy
               && ApproxDistanceSquareInMeters(newLocation.m_latitude,
                                               newLocation.m_longitude,
                                               m_prevLocation->m_latitude,
                                               m_prevLocation->m_longitude)
               > newLocation.m_horizontalAccuracy * newLocation.m_horizontalAccuracy)
        passes = false;
    }
    else
      m_prevLocation = newLocation;
    return passes;
  }
};

  class DesktopLocationService : public LocationService, public LocationObserver
  {
    std::vector<LocationService *> m_services;
    PositionFilter m_filter;
    bool m_reportFirstEvent;

    virtual void OnLocationError(location::TLocationError errorCode)
    {
      m_observer.OnLocationError(errorCode);
    }

    virtual void OnLocationUpdated(GpsInfo const & info)
    {
      if (m_filter.Passes(info))
        m_observer.OnLocationUpdated(info);
    }

  public:
    explicit DesktopLocationService(LocationObserver & observer)
      : LocationService(observer), m_reportFirstEvent(true)
    {
#if defined(OMIM_OS_MAC)
      m_services.push_back(CreateAppleLocationService(*this));
#endif
    }

    virtual ~DesktopLocationService()
    {
      for (size_t i = 0; i < m_services.size(); ++i)
        delete m_services[i];
    }

    virtual void Start()
    {
      for (size_t i = 0; i < m_services.size(); ++i)
        m_services[i]->Start();
    }

    virtual void Stop()
    {
      for (size_t i = 0; i < m_services.size(); ++i)
        m_services[i]->Stop();
      m_reportFirstEvent = true;
    }
  };
}  // namespace location

location::LocationService * CreateDesktopLocationService(location::LocationObserver & observer)
{
  return new location::DesktopLocationService(observer);
}
