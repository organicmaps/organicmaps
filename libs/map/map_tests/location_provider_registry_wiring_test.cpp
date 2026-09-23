#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "platform/location.hpp"
#include "platform/location_provider/location_provider_registry.hpp"

#include <utility>
#include <vector>

namespace location_provider_registry_wiring_tests
{
namespace
{
class TestFramework final : public Framework
{
public:
  TestFramework() : Framework({}, false /* loadMaps */) {}
};

// Records the native reading Resolve() was called with; returns it unchanged (passthrough), so
// this test only observes that the registry is wired into OnLocationUpdate's call path, without
// asserting on the unchanged, already-tested, asynchronous downstream pipeline
// (RoutingManager/Extrapolator/DrapeEngine).
class SpyStrategy final : public location_provider::LocationFusionStrategy
{
public:
  location::GpsInfo Resolve(
      location::GpsInfo const & native,
      std::vector<std::pair<std::string, location::GpsInfo>> const & /*candidates*/) override
  {
    m_lastNative = native;
    m_called = true;
    return native;
  }

  location::GpsInfo m_lastNative;
  bool m_called = false;
};
}  // namespace

UNIT_TEST(Framework_OnLocationUpdate_ResolvesThroughLocationProviderRegistry)
{
  SpyStrategy strategy;
  auto & registry = location_provider::LocationProviderRegistry::Instance();
  registry.SetFusionStrategy(&strategy);

  location::GpsInfo nativeInfo;
  nativeInfo.m_source = location::EAndroidNative;
  nativeInfo.m_latitude = 47.36667;
  nativeInfo.m_longitude = 8.55;
  nativeInfo.m_horizontalAccuracy = 12.5;

  TestFramework framework;
  framework.OnLocationUpdate(nativeInfo);

  // Restore default passthrough before any assertion can early-return, so a later test in this
  // binary never observes a stale strategy.
  registry.SetFusionStrategy(nullptr);

  TEST(strategy.m_called, ());
  TEST_EQUAL(strategy.m_lastNative.m_source, nativeInfo.m_source, ());
  TEST_ALMOST_EQUAL_ULPS(strategy.m_lastNative.m_latitude, nativeInfo.m_latitude, ());
  TEST_ALMOST_EQUAL_ULPS(strategy.m_lastNative.m_longitude, nativeInfo.m_longitude, ());
  TEST_ALMOST_EQUAL_ULPS(strategy.m_lastNative.m_horizontalAccuracy, nativeInfo.m_horizontalAccuracy, ());
}
}  // namespace location_provider_registry_wiring_tests
