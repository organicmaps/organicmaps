#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

namespace nogps
{
/// The car reports its speed about a second late: while it accelerates the calculated position falls behind
/// by the lag multiplied by the gained speed, and it runs ahead while the car brakes. The lag is measured by
/// trusted GPS while it works: the car speed is compared with the GPS speed of some time ago, the delay it
/// fits best at is the lag. It is kept between trips and compensated without GPS.
class SpeedLag
{
public:
  // The lag of the cars measured so far, used until GPS has measured this one.
  static double constexpr kDefaultLagSec = 1.2;
  static double constexpr kMaxLagSec = 3;
  static double constexpr kStepSec = 0.25;
  static int constexpr kSteps = static_cast<int>(kMaxLagSec / kStepSec) + 1;
  // A steady speed tells nothing about the lag.
  static double constexpr kMinAccelerationMps2 = 0.5;
  // The lag is trusted after this many samples of a changing speed, i.e. seconds.
  static int constexpr kMinSamples = 60;
  // Older samples are forgotten gradually, e.g. after another adapter is plugged in.
  static int constexpr kMaxSamples = 1200;
  // A longer pause between GPS samples is a break.
  static int64_t constexpr kMaxGpsGapMs = 2000;

  /// \param speedMps the car speed as the position is calculated with since |timeMs|.
  void OnCarSpeed(int64_t timeMs, double speedMps);
  /// \param timeMs when the speed was measured, in the time of OnCarSpeed().
  void OnGpsSpeed(int64_t timeMs, double speedMps);

  bool IsMeasured() const { return m_samples >= kMinSamples; }
  /// \returns how late the car reports its speed, seconds.
  double Get() const;

  void Clear();

  /// \returns the measurements to keep between trips.
  std::string Serialize() const;
  /// Restores the measurements kept before, broken ones are ignored.
  void Deserialize(std::string_view saved);

private:
  struct Sample
  {
    int64_t m_timeMs;
    double m_speedMps;
  };

  void Compare(Sample const & gps);
  // Returns nothing if the car speed was lost for a while.
  std::optional<double> CarSpeedAt(double timeMs) const;

  // The car speed since its time until the next one.
  std::deque<Sample> m_car;
  // GPS waiting for the car speed of kMaxLagSec later.
  std::deque<Sample> m_gps;
  int64_t m_lastGpsMs = 0;
  double m_lastGpsSpeedMps = 0;
  // Squared differences of the speeds for every lag.
  std::array<double, kSteps> m_errors{};
  double m_samples = 0;
};
}  // namespace nogps
