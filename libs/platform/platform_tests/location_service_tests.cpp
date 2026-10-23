#include "testing/testing.hpp"

#include "platform/location_service/location_service.hpp"
#include "platform/location_service/position_filter.hpp"

#include "std/target_os.hpp"

#include <ctime>
#include <memory>
#include <string>
#include <vector>

namespace location_service_tests
{
location::GpsInfo Fix(double timestamp)
{
  location::GpsInfo info;
  info.m_source = location::EGeoClue2;
  info.m_timestamp = timestamp;
  return info;
}

// The production wrapper is compiled with a fake native factory in this test target.
location::LocationObserver * g_providerObserver = nullptr;
int g_starts = 0;
int g_stops = 0;
bool g_createProvider = true;

class Provider : public location::LocationService
{
public:
  explicit Provider(location::LocationObserver & observer) : LocationService(observer)
  {
    g_providerObserver = &observer;
  }
  void Start() override { ++g_starts; }
  void Stop() override { ++g_stops; }
};

class Observer : public location::LocationObserver
{
public:
  void OnLocationError(location::TLocationError error) override { m_errors.push_back(error); }
  void OnLocationUpdated(location::GpsInfo const & info) override { m_locations.push_back(info); }
  std::vector<location::GpsInfo> m_locations;
  std::vector<location::TLocationError> m_errors;
};

UNIT_TEST(LocationFreshness_Boundaries)
{
  double const now = 1'800'000'000;
  TEST(location::IsCurrentLocation(Fix(now), {}, now), ());
  TEST(location::IsCurrentLocation(Fix(now - 300), {}, now), ());
  TEST(!location::IsCurrentLocation(Fix(now - 300.01), {}, now), ());
  TEST(!location::IsCurrentLocation(Fix(now - 86400), {}, now), ());
  TEST(location::IsCurrentLocation(Fix(now + 60), {}, now), ());
  TEST(location::IsCurrentLocation(Fix(now), Fix(now - 1), now), ());
  TEST(location::IsCurrentLocation(Fix(now), Fix(now), now), ());
  TEST(!location::IsCurrentLocation(Fix(now - 2), Fix(now - 1), now), ());
}

UNIT_TEST(DesktopLocationService_KeepsLastAcceptedFix)
{
  Observer observer;
  auto service = CreateDesktopLocationService(observer);
  TEST(g_providerObserver, ());
  double const now = static_cast<double>(std::time(nullptr));
  g_providerObserver->OnLocationUpdated(Fix(now));
  g_providerObserver->OnLocationUpdated(Fix(now + 2));
  g_providerObserver->OnLocationUpdated(Fix(now + 1));
  TEST_EQUAL(observer.m_locations.size(), 2, ());
  // A rejected update must not become the reference for the next one.
  g_providerObserver->OnLocationUpdated(Fix(now + 1.5));
  TEST_EQUAL(observer.m_locations.size(), 2, ());
  g_providerObserver->OnLocationUpdated(Fix(now + 2));
  TEST_EQUAL(observer.m_locations.size(), 3, ());
  g_providerObserver->OnLocationUpdated(Fix(now - 301));
  TEST_EQUAL(observer.m_locations.size(), 3, ());
}

UNIT_TEST(DesktopLocationService_ForwardsProviderEvents)
{
  Observer observer;
  auto service = CreateDesktopLocationService(observer);
  g_starts = g_stops = 0;
  service->Start();
  TEST_EQUAL(g_starts, 1, ());
  g_providerObserver->OnLocationError(location::EDenied);
  TEST_EQUAL(observer.m_errors.size(), 1, ());
  TEST_EQUAL(observer.m_errors.front(), location::EDenied, ());
  service->Stop();
  TEST_EQUAL(g_stops, 1, ());
  service->Start();
  TEST_EQUAL(g_starts, 2, ());
  service->Stop();
  TEST_EQUAL(g_stops, 2, ());
}
UNIT_TEST(DesktopLocationService_NoProvider)
{
  Observer observer;
  g_createProvider = false;
  auto service = CreateDesktopLocationService(observer);
  g_createProvider = true;
  g_starts = g_stops = 0;
  service->Start();
  service->Stop();
  TEST_EQUAL(g_starts, 0, ());
  TEST_EQUAL(g_stops, 0, ());
  TEST(observer.m_locations.empty(), ());
  TEST(observer.m_errors.empty(), ());
}
}  // namespace location_service_tests

#if defined(OMIM_OS_MAC)
std::unique_ptr<location::LocationService> CreateAppleLocationService(location::LocationObserver & observer)
#else
std::unique_ptr<location::LocationService> CreateQtLocationService(location::LocationObserver & observer,
                                                                   std::string const &)
#endif
{
  if (!location_service_tests::g_createProvider)
    return nullptr;
  return std::make_unique<location_service_tests::Provider>(observer);
}
