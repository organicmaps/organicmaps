#include "platform/location_service/location_service.hpp"
#include "platform/location_service/position_filter.hpp"

#include "std/target_os.hpp"

#include <ctime>
#include <optional>

#if defined(OMIM_OS_MAC)
std::unique_ptr<location::LocationService> CreateAppleLocationService(location::LocationObserver &);
#elif defined(QT_LOCATION_SERVICE)
std::unique_ptr<location::LocationService> CreateQtLocationService(location::LocationObserver &,
                                                                   std::string const & sourceName);
#endif

namespace location
{
class DesktopLocationService
  : public LocationService
  , public LocationObserver
{
  std::unique_ptr<LocationService> m_service;
  std::optional<GpsInfo> m_previous;

  void OnLocationError(location::TLocationError errorCode) override { m_observer.OnLocationError(errorCode); }

  void OnLocationUpdated(GpsInfo const & info) override
  {
    if (!IsCurrentLocation(info, m_previous, static_cast<double>(std::time(nullptr))))
      return;
    m_previous = info;
    m_observer.OnLocationUpdated(info);
  }

public:
  explicit DesktopLocationService(LocationObserver & observer) : LocationService(observer)
  {
#if defined(QT_LOCATION_SERVICE)
#if defined(OMIM_OS_LINUX)
    m_service = CreateQtLocationService(*this, "geoclue2");
#endif                                 // OMIM_OS_LINUX
#elif defined(APPLE_LOCATION_SERVICE)  // No QT_LOCATION_SERVICE
    m_service = CreateAppleLocationService(*this);
#endif                                 // QT_LOCATION_SERVICE
  }

  void Start() override
  {
    if (m_service)
      m_service->Start();
  }

  void Stop() override
  {
    if (m_service)
      m_service->Stop();
  }
};
}  // namespace location

std::unique_ptr<location::LocationService> CreateDesktopLocationService(location::LocationObserver & observer)
{
  return std::make_unique<location::DesktopLocationService>(observer);
}
