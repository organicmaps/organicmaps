#ifdef DEBUG

#include "testing/testing.hpp"

#include "map/location_provider/gpx_replay_fusion_strategy.hpp"
#include "map/location_provider/gpx_replay_provider.hpp"

#include "kml/types.hpp"

#include "geometry/mercator.hpp"

#include "platform/location.hpp"

#include <string>
#include <utility>
#include <vector>

namespace gpx_replay_fusion_strategy_tests
{
namespace
{
kml::TrackData MakeTrack(std::vector<ms::LatLon> const & points)
{
  kml::TrackData track;
  kml::TrackGeometry line;
  for (auto const & ll : points)
    line.emplace_back(mercator::FromLatLon(ll));
  track.m_geometry.m_lines.push_back(std::move(line));
  return track;
}

location::GpsInfo MakeNativeGpsInfo()
{
  location::GpsInfo native;
  native.m_source = location::EAndroidNative;
  native.m_latitude = 1.0;
  native.m_longitude = 2.0;
  return native;
}

// Mirrors how LocationProviderRegistry::Resolve() actually builds candidates: one
// (GetSourceId(), reading) pair per registered provider whose GetCurrentReading() returned a
// value this update; a std::nullopt reading is simply omitted.
std::vector<std::pair<std::string, location::GpsInfo>> BuildCandidates(location_provider::GpxReplayProvider & provider)
{
  std::vector<std::pair<std::string, location::GpsInfo>> candidates;
  if (auto const reading = provider.GetCurrentReading())
    candidates.emplace_back(provider.GetSourceId(), *reading);
  return candidates;
}
}  // namespace

UNIT_TEST(GpxReplayFusionStrategy_ArmedWithReading_ReplayWinsOverNative)
{
  location_provider::GpxReplayProvider provider;
  provider.Arm(MakeTrack({{47.0, 8.0}}), std::nullopt);

  location_provider::GpxReplayFusionStrategy strategy;
  auto const native = MakeNativeGpsInfo();
  auto const resolved = strategy.Resolve(native, BuildCandidates(provider));

  TEST_EQUAL(resolved.m_source, location::EPredictor, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_latitude, 47.0, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_longitude, 8.0, ());
}

UNIT_TEST(GpxReplayFusionStrategy_ArmedButExhausted_FallsBackToPassthrough)
{
  location_provider::GpxReplayProvider provider;
  provider.Arm(MakeTrack({{47.0, 8.0}}), std::nullopt);
  provider.GetCurrentReading();  // consume the single point

  location_provider::GpxReplayFusionStrategy strategy;
  auto const native = MakeNativeGpsInfo();
  // Exhaustion is driven by wall-clock time in GpxReplayProvider; here we bypass that by
  // directly simulating what the registry sees once GetCurrentReading() has returned nullopt —
  // an empty candidates vector, which is exactly what BuildCandidates() would produce whenever
  // the provider has nothing to offer, regardless of the reason.
  std::vector<std::pair<std::string, location::GpsInfo>> const emptyCandidates;
  auto const resolved = strategy.Resolve(native, emptyCandidates);

  TEST_EQUAL(resolved.m_source, native.m_source, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_latitude, native.m_latitude, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_longitude, native.m_longitude, ());
}

UNIT_TEST(GpxReplayFusionStrategy_Disarmed_Passthrough)
{
  location_provider::GpxReplayProvider provider;
  TEST(!provider.IsArmed(), ());

  location_provider::GpxReplayFusionStrategy strategy;
  auto const native = MakeNativeGpsInfo();
  auto const resolved = strategy.Resolve(native, BuildCandidates(provider));

  TEST_EQUAL(resolved.m_source, native.m_source, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_latitude, native.m_latitude, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_longitude, native.m_longitude, ());
}
}  // namespace gpx_replay_fusion_strategy_tests

#endif  // DEBUG
