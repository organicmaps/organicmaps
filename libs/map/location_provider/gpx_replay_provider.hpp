#pragma once

#ifdef DEBUG

#include "platform/location.hpp"
#include "platform/location_provider/location_provider.hpp"

#include "kml/types.hpp"

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace location_provider
{
// Debug-only reference LocationProvider: replays a GPX track's first line as a sequence of
// simulated GpsInfo readings, interpolated along the track by real-world distance at a fixed
// constant speed. Armed/disarmed via the `?mock-gpx:`/`?mock-gpx-stop` debug commands (see
// Framework::ParseMockGpxCommand); never touches LocationProviderRegistry itself — see
// design.md's registration-model decision.
class GpxReplayProvider : public LocationProvider
{
public:
  // A typical hiking pace, chosen so a replayed track is trackable on the map as it plays back
  // rather than finishing before the map has a chance to center on it.
  static constexpr double kDefaultSpeedMetersPerSecond = 3.0 * 1609.344 / 3600.0;
  // Held at the first point for this long after Arm() so a human has time to find the starting
  // position on the map before it starts moving.
  static constexpr double kStartDelaySeconds = 5.0;
  static constexpr double kDefaultAccuracyMeters = 5.0;
  static constexpr char const * kSourceId = "gpx_replay";

  // The process-wide instance registered with LocationProviderRegistry at debug-build
  // static-init time (see gpx_replay_fusion_strategy.cpp). Framework::ParseMockGpxCommand /
  // ParseMockGpxStopCommand call Arm()/Disarm() on this same instance directly — they never touch
  // the registry itself, per design.md's registration-model decision.
  static GpxReplayProvider & Instance();

  std::string GetSourceId() const override { return kSourceId; }

  // track.m_geometry.m_lines[0] must be non-empty. Re-arms (replace, not append) if already
  // armed, resetting playback to the new track's first point.
  void Arm(kml::TrackData const & track, std::optional<double> accuracyOverride);
  void Disarm();
  bool IsArmed() const { return m_armed; }

  // Held at the track's first point for kStartDelaySeconds after Arm(), then moves along the
  // track at kDefaultSpeedMetersPerSecond, interpolating position by real-world distance rather
  // than stepping through raw trackpoints. std::nullopt once past the last point (freeze, no
  // loop) — including for a single-point track, returned exactly once before freezing.
  std::optional<location::GpsInfo> GetCurrentReading() override;

  // Non-mutating: true exactly when the next GetCurrentReading() call would return a value
  // rather than std::nullopt. Lets a caller (e.g. a replay-driving ticker) know when to stop
  // polling without consuming a reading just to check.
  bool HasMoreReadings() const { return m_armed && !m_finished; }

  // Test-only: overrides the effective start delay / speed used by GetCurrentReading(), so unit
  // tests can exercise time-driven playback without multi-second real-world sleeps. Takes effect
  // immediately, but only until the next Arm() call, which always resets to the production
  // defaults above; production code never calls this.
  void SetTimingForTest(double startDelaySeconds, double speedMetersPerSecond);

  // Test-only: exposes the pure distance-along-track -> position interpolation used internally,
  // decoupled from wall-clock timing, so tests can check interpolation math deterministically.
  // Must be called after Arm(); production code never calls this.
  location::GpsInfo GetReadingAtDistanceForTest(double distanceMeters) const
  {
    return SynthesizeGpsInfoAtDistance(distanceMeters);
  }

private:
  location::GpsInfo SynthesizeGpsInfoAtDistance(double distanceMeters) const;

  bool m_armed = false;
  bool m_finished = false;
  kml::TrackGeometry m_line;
  // Parallel to m_line; m_cumulativeDistanceMeters[i] is the real-world distance from m_line[0]
  // to m_line[i] along the track. [0] is always 0.
  std::vector<double> m_cumulativeDistanceMeters;
  std::optional<double> m_accuracyOverride;
  std::chrono::steady_clock::time_point m_armTime;
  double m_startDelaySeconds = kStartDelaySeconds;
  double m_speedMetersPerSecond = kDefaultSpeedMetersPerSecond;
};
}  // namespace location_provider

#endif  // DEBUG
