#ifdef DEBUG

#include "testing/testing.hpp"

#include "map/location_provider/gpx_replay_provider.hpp"

#include "kml/types.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "platform/location.hpp"

#include <chrono>
#include <thread>

namespace gpx_replay_provider_tests
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

// A short real-world sleep, comfortably longer than the tiny test start delays used below, so a
// timing-gated transition (end of hold, or well past the end of a short track) is guaranteed to
// have become eligible without coupling the test to an exact boundary.
void SleepPast(double seconds)
{
  std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
}
}  // namespace

UNIT_TEST(GpxReplayProvider_SourceIdIsGpxReplay)
{
  location_provider::GpxReplayProvider provider;
  TEST_EQUAL(provider.GetSourceId(), location_provider::GpxReplayProvider::kSourceId, ());
}

UNIT_TEST(GpxReplayProvider_FirstReadingAfterArmReturnsFirstPointImmediately)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}, {47.002, 8.002}});
  provider.Arm(track, std::nullopt);

  auto const reading = provider.GetCurrentReading();
  TEST(reading.has_value(), ());
  TEST_ALMOST_EQUAL_ULPS(reading->m_latitude, 47.0, ());
  TEST_ALMOST_EQUAL_ULPS(reading->m_longitude, 8.0, ());
  TEST_EQUAL(reading->m_source, location::EPredictor, ());
  TEST_ALMOST_EQUAL_ULPS(reading->m_horizontalAccuracy, location_provider::GpxReplayProvider::kDefaultAccuracyMeters, ());
}

UNIT_TEST(GpxReplayProvider_HoldsAtFirstPointDuringStartDelay)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}});
  provider.Arm(track, std::nullopt);
  provider.SetTimingForTest(/*startDelaySeconds=*/10.0, /*speedMetersPerSecond=*/1000.0);

  auto const first = provider.GetCurrentReading();
  auto const second = provider.GetCurrentReading();
  TEST(first.has_value(), ());
  TEST(second.has_value(), ());
  TEST_ALMOST_EQUAL_ULPS(first->m_latitude, 47.0, ());
  TEST_ALMOST_EQUAL_ULPS(first->m_longitude, 8.0, ());
  TEST_ALMOST_EQUAL_ULPS(second->m_latitude, first->m_latitude, ());
  TEST_ALMOST_EQUAL_ULPS(second->m_longitude, first->m_longitude, ());
}

UNIT_TEST(GpxReplayProvider_AdvancesAlongTrackAfterStartDelay)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.1, 8.1}});
  provider.Arm(track, std::nullopt);
  // A tiny delay and a slow-enough speed that the track (tens of km) is nowhere near finished
  // after a short real sleep, so this only proves motion is happening, not exact position.
  provider.SetTimingForTest(/*startDelaySeconds=*/0.05, /*speedMetersPerSecond=*/1000.0);

  auto const atStart = provider.GetCurrentReading();
  SleepPast(0.15);
  auto const afterDelay = provider.GetCurrentReading();

  TEST(atStart.has_value(), ());
  TEST(afterDelay.has_value(), ());
  TEST(provider.HasMoreReadings(), ());
  TEST(afterDelay->m_latitude > atStart->m_latitude, ());
  TEST(afterDelay->m_latitude < 47.1, ());
  TEST(afterDelay->m_longitude > atStart->m_longitude, ());
  TEST(afterDelay->m_longitude < 8.1, ());
}

UNIT_TEST(GpxReplayProvider_FreezesToNulloptOnceTrackIsFullyWalked)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}});
  provider.Arm(track, std::nullopt);
  // Fast enough that even a short real sleep covers the whole (~130m) track.
  provider.SetTimingForTest(/*startDelaySeconds=*/0.0, /*speedMetersPerSecond=*/10000.0);

  SleepPast(0.05);
  auto const finished = provider.GetCurrentReading();
  TEST(finished.has_value(), ());
  TEST_ALMOST_EQUAL_ULPS(finished->m_latitude, 47.001, ());
  TEST_ALMOST_EQUAL_ULPS(finished->m_longitude, 8.001, ());

  auto const pastEnd = provider.GetCurrentReading();
  TEST(!pastEnd.has_value(), ());
}

UNIT_TEST(GpxReplayProvider_SinglePointTrackReturnsOnceThenNullopt)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}});
  provider.Arm(track, std::nullopt);
  provider.SetTimingForTest(/*startDelaySeconds=*/0.0, /*speedMetersPerSecond=*/1000.0);

  auto const first = provider.GetCurrentReading();
  TEST(first.has_value(), ());
  TEST_ALMOST_EQUAL_ULPS(first->m_latitude, 47.0, ());

  SleepPast(0.05);
  auto const second = provider.GetCurrentReading();
  TEST(!second.has_value(), ());
}

UNIT_TEST(GpxReplayProvider_AccuracyOverrideAppliedWhenProvided)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}});
  provider.Arm(track, 42.0);

  auto const reading = provider.GetCurrentReading();
  TEST(reading.has_value(), ());
  TEST_ALMOST_EQUAL_ULPS(reading->m_horizontalAccuracy, 42.0, ());
}

UNIT_TEST(GpxReplayProvider_DisarmStopsReadings)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}});
  provider.Arm(track, std::nullopt);
  TEST(provider.IsArmed(), ());

  provider.Disarm();
  TEST(!provider.IsArmed(), ());
  TEST(!provider.GetCurrentReading().has_value(), ());
}

UNIT_TEST(GpxReplayProvider_HasMoreReadings_FalseBeforeArm)
{
  location_provider::GpxReplayProvider provider;
  TEST(!provider.HasMoreReadings(), ());
}

UNIT_TEST(GpxReplayProvider_HasMoreReadings_TrueWhileArmedWithinBounds)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}});
  provider.Arm(track, std::nullopt);

  TEST(provider.HasMoreReadings(), ());
  provider.GetCurrentReading();
  TEST(provider.HasMoreReadings(), ());
}

UNIT_TEST(GpxReplayProvider_HasMoreReadings_FalseAfterLastPointConsumed)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}});
  provider.Arm(track, std::nullopt);
  provider.SetTimingForTest(/*startDelaySeconds=*/0.0, /*speedMetersPerSecond=*/10000.0);

  SleepPast(0.05);
  provider.GetCurrentReading();  // consumes the final, exhausting reading

  TEST(!provider.HasMoreReadings(), ());
}

UNIT_TEST(GpxReplayProvider_HasMoreReadings_FalseAfterDisarm)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}});
  provider.Arm(track, std::nullopt);
  TEST(provider.HasMoreReadings(), ());

  provider.Disarm();
  TEST(!provider.HasMoreReadings(), ());
}

UNIT_TEST(GpxReplayProvider_ReArmingMidPlaybackResetsToNewTracksFirstPoint)
{
  location_provider::GpxReplayProvider provider;
  auto const trackA = MakeTrack({{47.0, 8.0}, {47.001, 8.001}});
  provider.Arm(trackA, std::nullopt);
  provider.SetTimingForTest(/*startDelaySeconds=*/0.0, /*speedMetersPerSecond=*/10000.0);
  SleepPast(0.05);
  provider.GetCurrentReading();  // trackA fully walked

  auto const trackB = MakeTrack({{10.0, 20.0}, {10.001, 20.001}});
  provider.Arm(trackB, std::nullopt);

  auto const reading = provider.GetCurrentReading();
  TEST(reading.has_value(), ());
  TEST_ALMOST_EQUAL_ULPS(reading->m_latitude, 10.0, ());
  TEST_ALMOST_EQUAL_ULPS(reading->m_longitude, 20.0, ());
}

UNIT_TEST(GpxReplayProvider_InterpolatesLinearlyBetweenBracketingPoints)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}, {47.002, 8.002}});
  provider.Arm(track, std::nullopt);

  double const total = mercator::DistanceOnEarth(mercator::FromLatLon({47.0, 8.0}), mercator::FromLatLon({47.002, 8.002}));
  double const firstSeg = mercator::DistanceOnEarth(mercator::FromLatLon({47.0, 8.0}), mercator::FromLatLon({47.001, 8.001}));

  auto const atStart = provider.GetReadingAtDistanceForTest(0.0);
  TEST_ALMOST_EQUAL_ULPS(atStart.m_latitude, 47.0, ());
  TEST_ALMOST_EQUAL_ULPS(atStart.m_longitude, 8.0, ());

  auto const atFirstBoundary = provider.GetReadingAtDistanceForTest(firstSeg);
  TEST_ALMOST_EQUAL_ABS(atFirstBoundary.m_latitude, 47.001, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(atFirstBoundary.m_longitude, 8.001, 1e-6, ());

  auto const atEnd = provider.GetReadingAtDistanceForTest(total);
  TEST_ALMOST_EQUAL_ABS(atEnd.m_latitude, 47.002, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(atEnd.m_longitude, 8.002, 1e-6, ());

  auto const midFirstSeg = provider.GetReadingAtDistanceForTest(firstSeg / 2.0);
  TEST(midFirstSeg.m_latitude > 47.0 && midFirstSeg.m_latitude < 47.001, ());
  TEST(midFirstSeg.m_longitude > 8.0 && midFirstSeg.m_longitude < 8.001, ());
}

UNIT_TEST(GpxReplayProvider_ClampsDistanceBeyondTrackToLastPoint)
{
  location_provider::GpxReplayProvider provider;
  auto const track = MakeTrack({{47.0, 8.0}, {47.001, 8.001}});
  provider.Arm(track, std::nullopt);

  double const total = mercator::DistanceOnEarth(mercator::FromLatLon({47.0, 8.0}), mercator::FromLatLon({47.001, 8.001}));
  auto const wayPastEnd = provider.GetReadingAtDistanceForTest(total * 10.0);
  TEST_ALMOST_EQUAL_ABS(wayPastEnd.m_latitude, 47.001, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(wayPastEnd.m_longitude, 8.001, 1e-6, ());
}
}  // namespace gpx_replay_provider_tests

#endif  // DEBUG
