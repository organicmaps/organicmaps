#ifdef DEBUG

// Verifies TASK-007: in a Debug build, GpxReplayProvider/GpxReplayFusionStrategy self-register
// with the process-wide LocationProviderRegistry at static-init time (see
// gpx_replay_fusion_strategy.cpp), before this test ever runs — no explicit setup needed here.
// This lives in map_tests, not platform_tests, because the static registration itself is a
// libs/map translation unit (gpx_replay_fusion_strategy.cpp) that platform_tests cannot link
// against without inverting libs/platform's layering (see design.md v3.2 Key Decisions) — the
// same reason TASK-005/006's own tests live here instead of platform_tests.

#include "testing/testing.hpp"

#include "platform/location.hpp"
#include "platform/location_provider/location_provider_registry.hpp"

namespace gpx_replay_debug_registration_tests
{
UNIT_TEST(LocationProviderRegistry_DebugBuildDefaultsToNativePassthroughWhenNothingArmed)
{
  location::GpsInfo native;
  native.m_source = location::EAndroidNative;
  native.m_latitude = 12.34;
  native.m_longitude = 56.78;
  native.m_horizontalAccuracy = 10.0;

  auto const resolved = location_provider::LocationProviderRegistry::Instance().Resolve(native);

  TEST_EQUAL(resolved.m_source, native.m_source, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_latitude, native.m_latitude, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_longitude, native.m_longitude, ());
  TEST_ALMOST_EQUAL_ULPS(resolved.m_horizontalAccuracy, native.m_horizontalAccuracy, ());
}
}  // namespace gpx_replay_debug_registration_tests

#endif  // DEBUG
