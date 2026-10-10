#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace nogps
{
/// The car reports its speed about a second late, an accelerometer fixed in the car feels the acceleration at
/// once. It tells how much faster the car is now than it reports: the acceleration along the car summed over
/// the time of the lag.
///
/// Nothing is known about how the sensor is mounted. The direction along the car is the one the acceleration
/// of the sensor follows the changes of the car speed in, it is learned on the way. The constant part of the
/// acceleration, i.e. the gravity along that direction on a slope or with a tilted sensor, is taken from the
/// last seconds against the car speed. If the sensor is moved, e.g. the phone is taken from its holder, the
/// accelerometer stops following the car and is not used until the direction is learned again.
class AccelSpeed
{
public:
  using Vec3 = std::array<double, 3>;

  struct Params
  {
    // The samples of the sensor are averaged over this time.
    int64_t m_sliceMs = 100;
    // The acceleration of the car is the change of its speed over this time.
    int64_t m_carStepMs = 1000;
    // The sensor is compared with the car this often, while the car speed changes at least this fast: a
    // steady speed tells nothing, and the car rounds it to 1 km/h.
    int64_t m_learnIntervalMs = 500;
    double m_minCarAccelerationMps2 = 0.5;
    // The comparisons are forgotten gradually, the memory is this many of them: 2 min of speeding up and
    // braking.
    double m_memory = 240;
    // The direction is trusted after this many comparisons with the sensor following the car well.
    double m_minSamples = 40;
    double m_minCorrelation = 0.75;
    // The constant part of the acceleration is taken from this time before the lag.
    int64_t m_biasMs = 5000;
    int64_t m_minBiasMs = 2500;
    // The last comparisons, about this many, tell if the sensor still follows the car: its acceleration along
    // the car changes together with the car speed and as much as it. If it doesn't at all, the sensor has
    // been moved, and the direction is learned anew.
    double m_checkMemory = 20;
    double m_minCheckSamples = 10;
    double m_minRecentCorrelation = 0.5;
    double m_minRecentScale = 0.6;
    double m_maxRecentScale = 1.6;
    double m_movedCorrelation = 0.2;
    double m_movedScale = 0.3;
    // No car accelerates faster.
    double m_maxAccelerationMps2 = 4.5;
    // A longer silence of the sensor or of the car speed is a break.
    int64_t m_maxAccelGapMs = 1000;
    int64_t m_maxSpeedGapMs = 2500;
  };

  AccelSpeed() = default;
  explicit AccelSpeed(Params const & params) : m_params(params) {}

  /// Forgets everything: another sensor.
  void Reset();

  /// What is learned about how the sensor is fixed in the car. A sensor box stays in its place: the application
  /// started again goes on with it, and knows which way the car starts from the first minute of a trip.
  std::string Serialize() const;
  /// \returns false if the string is wrong, nothing is changed then.
  bool Deserialize(std::string const & data);
  /// A maneuvering car is not learned from: it drives backwards too, and its speed has no sign.
  void SetLearning(bool learning) { m_learning = learning; }

  /// \param lagSec how late the car reports its speed.
  void SetLag(double lagSec) { m_lagMs = static_cast<int64_t>(lagSec * 1000); }

  /// \param acceleration along three fixed axes of the sensor, m/s². The gravity may be in it or not.
  void OnAccel(int64_t timeMs, Vec3 const & acceleration);

  /// \param speedMps the speed the car reports at |timeMs|, in the time of OnAccel().
  void OnCarSpeed(int64_t timeMs, double speedMps);

  /// \returns how much faster the car is at |nowMs| than it reports, m/s, if the accelerometer is known to
  /// follow the car.
  std::optional<double> GetGain(int64_t nowMs) const;

  /// \returns the mean acceleration along the car in (fromMs, toMs] with its constant part, the gravity on a slope.
  /// Nothing if the sensor doesn't follow the car or was silent for a part of the time.
  std::optional<double> GetAccelAlong(int64_t fromMs, int64_t toMs) const;

  /// \returns true if the direction along the car is known and the sensor follows the car.
  bool IsUsable() const;
  /// \returns how well the sensor follows the changes of the car speed, 1 is the best.
  double GetCorrelation() const { return m_correlation; }
  /// \returns the same of the last comparisons only.
  double GetRecentCorrelation() const;
  /// \returns how much the acceleration of the sensor along the car changes when the one of the car changes
  /// by 1, in the last comparisons: 1 if the sensor follows the car.
  double GetRecentScale() const;
  double GetSamples() const { return m_n; }

private:
  struct Slice
  {
    int64_t m_endMs;
    int64_t m_durationMs;
    Vec3 m_acceleration;
  };

  // The car speed since its time until the next one.
  struct Speed
  {
    int64_t m_timeMs;
    double m_speedMps;
  };

  void Learn(int64_t timeMs);
  void UpdateDirection();
  // Forgets the direction along the car, the last seconds of the sensor and of the car speed are kept.
  void Forget();
  // The car speed reported at the time, nothing if it is not known then.
  std::optional<double> CarSpeedAt(int64_t timeMs) const;
  // The mean acceleration of the sensor in (fromMs, toMs], nothing if the sensor was silent for a part of it.
  std::optional<Vec3> MeanAccel(int64_t fromMs, int64_t toMs) const;
  // The constant part of the acceleration along the car by the time the car speed is known at.
  std::optional<double> GetBias(int64_t carTimeMs) const;

  Params m_params;
  int64_t m_lagMs = 1200;

  std::deque<Slice> m_slices;
  std::deque<Speed> m_speeds;
  // When the car has reported its speed last.
  int64_t m_lastSpeedMs = 0;
  // The slice being collected.
  int64_t m_sliceStartMs = 0;
  int64_t m_lastAccelMs = 0;
  int m_sliceCount = 0;
  Vec3 m_sliceSum{};
  int64_t m_lastLearnMs = 0;

  // The sums of the comparisons, x is the acceleration of the sensor, y is the one of the car.
  double m_n = 0;
  Vec3 m_sumX{};
  double m_sumY = 0;
  Vec3 m_sumXY{};
  // xx, xy, xz, yy, yz, zz of the sensor.
  std::array<double, 6> m_sumXX{};
  double m_sumYY = 0;

  bool m_learning = true;
  bool m_hasDirection = false;
  Vec3 m_direction{};
  double m_correlation = 0;

  // The sums of the last comparisons, a is the acceleration of the sensor along the car.
  double m_checkN = 0;
  double m_checkA = 0;
  double m_checkY = 0;
  double m_checkAY = 0;
  double m_checkAA = 0;
  double m_checkYY = 0;
};
}  // namespace nogps
