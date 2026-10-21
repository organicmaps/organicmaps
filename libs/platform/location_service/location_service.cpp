#include "platform/location_service/location_service.hpp"

#if defined(APPLE_LOCATION_SERVICE)
std::unique_ptr<location::LocationService> CreateAppleLocationService(location::LocationObserver &);
#elif defined(QT_LOCATION_SERVICE)
std::unique_ptr<location::LocationService> CreateQtLocationService(location::LocationObserver &);
#endif

namespace location
{
class UnavailableLocationService final : public LocationService
{
public:
  using LocationService::LocationService;
  void Start() override {}
  void Stop() override {}
};
}  // namespace location

std::unique_ptr<location::LocationService> CreateDesktopLocationService(location::LocationObserver & observer)
{
#if defined(QT_LOCATION_SERVICE)
  return CreateQtLocationService(observer);
#elif defined(APPLE_LOCATION_SERVICE)
  return CreateAppleLocationService(observer);
#else
  return std::make_unique<location::UnavailableLocationService>(observer);
#endif
}
