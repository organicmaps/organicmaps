#include "testing/testing.hpp"

#include "platform/location_provider/location_provider_registry.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
using namespace location_provider;

location::GpsInfo MakeGpsInfo(double lat)
{
  location::GpsInfo info;
  info.m_source = location::EAndroidNative;
  info.m_latitude = lat;
  info.m_longitude = 0.0;
  return info;
}

class FakeProvider : public LocationProvider
{
public:
  FakeProvider(std::string sourceId, std::optional<location::GpsInfo> reading)
    : m_sourceId(std::move(sourceId))
    , m_reading(reading)
  {}

  std::string GetSourceId() const override { return m_sourceId; }
  std::optional<location::GpsInfo> GetCurrentReading() override { return m_reading; }

private:
  std::string m_sourceId;
  std::optional<location::GpsInfo> m_reading;
};

// Records the native reading it was called with and returns a fixed, distinguishable result.
class SpyStrategy : public LocationFusionStrategy
{
public:
  explicit SpyStrategy(location::GpsInfo result) : m_result(result) {}

  location::GpsInfo Resolve(location::GpsInfo const & native,
                             std::vector<std::pair<std::string, location::GpsInfo>> const & candidates) override
  {
    m_lastNative = native;
    m_lastCandidates = candidates;
    return m_result;
  }

  location::GpsInfo m_lastNative;
  std::vector<std::pair<std::string, location::GpsInfo>> m_lastCandidates;

private:
  location::GpsInfo m_result;
};
}  // namespace

UNIT_TEST(LocationProviderRegistry_EmptyRegistryPassthrough)
{
  LocationProviderRegistry registry;
  auto const native = MakeGpsInfo(1.0);
  auto const resolved = registry.Resolve(native);
  TEST_ALMOST_EQUAL_ULPS(resolved.m_latitude, native.m_latitude, ());
  TEST_EQUAL(resolved.m_source, native.m_source, ());
}

UNIT_TEST(LocationProviderRegistry_SingleProviderAndStrategyRoundTrip)
{
  LocationProviderRegistry registry;
  auto const providerReading = MakeGpsInfo(2.0);
  FakeProvider provider("test_source", providerReading);
  registry.RegisterProvider(&provider);

  auto const strategyResult = MakeGpsInfo(3.0);
  SpyStrategy strategy(strategyResult);
  registry.SetFusionStrategy(&strategy);

  auto const native = MakeGpsInfo(1.0);
  auto const resolved = registry.Resolve(native);

  TEST_ALMOST_EQUAL_ULPS(resolved.m_latitude, strategyResult.m_latitude, ());
  TEST_ALMOST_EQUAL_ULPS(strategy.m_lastNative.m_latitude, native.m_latitude, ());
  TEST_EQUAL(strategy.m_lastCandidates.size(), 1, ());
  TEST_EQUAL(strategy.m_lastCandidates[0].first, "test_source", ());
  TEST_ALMOST_EQUAL_ULPS(strategy.m_lastCandidates[0].second.m_latitude, providerReading.m_latitude, ());
}

UNIT_TEST(LocationProviderRegistry_NulloptReadingExcludedFromCandidates)
{
  LocationProviderRegistry registry;
  FakeProvider provider("no_reading", std::nullopt);
  registry.RegisterProvider(&provider);

  SpyStrategy strategy(MakeGpsInfo(0.0));
  registry.SetFusionStrategy(&strategy);

  registry.Resolve(MakeGpsInfo(1.0));
  TEST(strategy.m_lastCandidates.empty(), ());
}

UNIT_TEST(LocationProviderRegistry_MultipleProvidersAllAppearInCandidates)
{
  LocationProviderRegistry registry;
  FakeProvider providerA("source_a", MakeGpsInfo(10.0));
  FakeProvider providerB("source_b", MakeGpsInfo(20.0));
  registry.RegisterProvider(&providerA);
  registry.RegisterProvider(&providerB);

  SpyStrategy strategy(MakeGpsInfo(0.0));
  registry.SetFusionStrategy(&strategy);

  registry.Resolve(MakeGpsInfo(1.0));
  TEST_EQUAL(strategy.m_lastCandidates.size(), 2, ());
  TEST_EQUAL(strategy.m_lastCandidates[0].first, "source_a", ());
  TEST_EQUAL(strategy.m_lastCandidates[1].first, "source_b", ());
}

UNIT_TEST(LocationProviderRegistry_SecondSetFusionStrategyWins)
{
  LocationProviderRegistry registry;
  SpyStrategy first(MakeGpsInfo(100.0));
  SpyStrategy second(MakeGpsInfo(200.0));
  registry.SetFusionStrategy(&first);
  registry.SetFusionStrategy(&second);

  auto const resolved = registry.Resolve(MakeGpsInfo(1.0));
  TEST_ALMOST_EQUAL_ULPS(resolved.m_latitude, 200.0, ());
}
