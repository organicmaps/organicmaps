#pragma once

#include "map/nogps/elm327_session.hpp"
#include "map/nogps/gyro_calibrator.hpp"
#include "map/nogps/motion_source.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace nogps
{
class Clock;
class Delegate;
class Scheduler;

/// The phone fixed on the dashboard: turns come from its gyroscope, the car speed comes from an ELM327
/// OBD-II adapter. The gyroscope is calibrated by the user and automatically on every stop.
class PhoneMotion
  : public MotionSource
  , private Elm327Session::Listener
{
public:
  static double constexpr kMaxSensorDtSec = 0.1;
  // Wait after the car stops before the automatic calibration, the car body still sways.
  static int64_t constexpr kAutoCalibrationDelayMs = 2000;
  // Without fresh speed the car may move, the automatic calibration waits.
  static int64_t constexpr kSpeedStaleMs = 2000;

  /// \param elm327Address the address of the ELM327 adapter, nothing if it is not chosen yet.
  PhoneMotion(Delegate & delegate, Scheduler & scheduler, Clock const & clock, std::optional<std::string> elm327Address,
              MotionSource::Listener & listener);

  // MotionSource overrides:
  void Start() override;
  void Stop() override;
  SourceState GetState() const override;
  bool IsCalibrated() const override { return m_bias && m_up; }
  CalibrationState GetCalibrationState() override { return m_calibrationState; }
  int GetCalibrationProgressPercent() const override { return m_calibrator.GetProgressPercent(); }
  void Calibrate() override;
  std::string GetDeviceName() const override { return {}; }

  /// \param gyro the rotation rate in the phone axes, rad/s, counter-clockwise positive (as Android reports).
  void OnGyro(int64_t timestampNs, Vec3 const & gyro);
  /// \param accel the acceleration in the phone axes, m/s², the reaction to gravity points up.
  void OnAccel(Vec3 const & accel);

  Elm327Session * GetElm327() { return m_elm327.get(); }

private:
  // Elm327Session::Listener overrides:
  void OnSpeed(int speedKmh, int64_t timeMs) override;

  bool IsCarMoving(int64_t nowMs) const;
  void UpdateCalibration(Vec3 const & gyro);
  void ApplyAutoCalibration(int64_t stoppedMs);

  Delegate & m_delegate;
  Scheduler & m_scheduler;
  Clock const & m_clock;
  std::optional<std::string> const m_elm327Address;
  MotionSource::Listener & m_listener;
  std::unique_ptr<Elm327Session> m_elm327;

  GyroCalibrator m_calibrator;
  GyroCalibrator m_autoCalibrator;
  CalibrationState m_calibrationState = CalibrationState::None;
  // The car has driven during the calibration asked by the user.
  bool m_calibrationMoved = false;
  // The last automatic calibration at the current stop, it is used when the next one confirms it.
  std::optional<Vec3> m_previousAutoBias;
  std::optional<Vec3> m_bias;
  std::optional<Vec3> m_up;

  std::optional<Vec3> m_accel;
  int64_t m_lastGyroTimestampNs = 0;

  int m_speedKmh = -1;
  int64_t m_speedTimeMs = 0;
  // Since when the car stands, nothing while it drives.
  std::optional<int64_t> m_stoppedSinceMs;
};
}  // namespace nogps
