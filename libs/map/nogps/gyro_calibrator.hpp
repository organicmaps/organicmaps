#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace nogps
{
using Vec3 = std::array<double, 3>;

/// Calibrates the phone gyroscope while the car stands still: the mean rotation rate is the gyroscope bias,
/// and the mean acceleration points up (the accelerometer measures the reaction to gravity).
/// The phone is fixed on the dashboard, so the "up" direction in the phone axes doesn't change after calibration,
/// and the car yaw rate is the rotation around it whatever the phone orientation is.
class GyroCalibrator
{
public:
  // Samples needed for the calibration, ~3 s at 50 Hz.
  static int constexpr kRequiredSamples = 150;
  // Rotation noise of a still phone on a running engine is ~0.01 rad/s.
  static double constexpr kMaxStillGyroStdRadS = 0.03;
  static double constexpr kMaxStillAccelStdMps2 = 0.5;
  // The car creeping in a jam or maneuvering is often reported standing by the car, and its slow steady turn looks
  // like a still phone. The bias changes slowly with the temperature, so a larger change at a short stop is such a
  // turn: it would rotate the heading all the way until the next stop.
  static double constexpr kMaxBiasChangeDegS = 0.5;
  static int64_t constexpr kLongStopMs = 15'000;
  // The last calibration of a stop may catch the car starting off while the car still tells it stands. A
  // calibration is used when the next one at the same stop agrees with it, the noise of a still phone differs by
  // less.
  static double constexpr kMaxCalibrationsDiffDegS = 0.3;

  void Reset();
  /// \param gyro rotation rate in the phone axes, rad/s, counter-clockwise positive (as Android reports).
  /// \param accel acceleration in the phone axes, m/s², the reaction to gravity points up (as Android reports).
  void Add(Vec3 const & gyro, Vec3 const & accel);

  bool IsComplete() const { return m_count >= kRequiredSamples; }
  int GetProgressPercent() const;
  /// \returns true if the phone didn't move during the calibration.
  bool IsStill() const;
  Vec3 GetBias() const;
  /// \returns the unit vector pointing up in the phone axes.
  Vec3 GetUp() const;

  /// \param oldBias the bias in use, nothing if there is none.
  /// \param stoppedMs how long the car stands.
  /// \returns true if the new bias of a calibration at a stop can be trusted.
  static bool IsBiasChangeAllowed(std::optional<Vec3> const & oldBias, Vec3 const & newBias, Vec3 const & up,
                                  int64_t stoppedMs);
  /// \returns the mean of two calibrations made one after another at a stop, nothing if they disagree.
  static std::optional<Vec3> Confirm(Vec3 const & previousBias, Vec3 const & bias, Vec3 const & up);
  /// \returns the car yaw rate, deg/s, clockwise positive (as the compass heading grows).
  static double YawRateDeg(Vec3 const & gyro, Vec3 const & bias, Vec3 const & up);

private:
  double Std(double sum, double sqSum) const;

  Vec3 m_gyroSum{};
  Vec3 m_gyroSqSum{};
  Vec3 m_accelSum{};
  Vec3 m_accelSqSum{};
  int m_count = 0;
};
}  // namespace nogps
