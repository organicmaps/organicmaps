#include "map/nogps/phone_motion.hpp"

#include "map/nogps/clock.hpp"

#include "base/logging.hpp"

#include <cmath>

namespace nogps
{
PhoneMotion::PhoneMotion(Delegate & delegate, Scheduler & scheduler, Clock const & clock,
                         std::optional<std::string> elm327Address, MotionSource::Listener & listener)
  : m_delegate(delegate)
  , m_scheduler(scheduler)
  , m_clock(clock)
  , m_elm327Address(std::move(elm327Address))
  , m_listener(listener)
{}

void PhoneMotion::Start()
{
  LOG(LINFO, ("ELM327 =", m_elm327Address));
  if (m_elm327Address)
  {
    Elm327Session::Listener & listener = *this;
    m_elm327 = std::make_unique<Elm327Session>(m_delegate, m_scheduler, m_clock, *m_elm327Address, listener);
    m_elm327->Start();
  }
}

void PhoneMotion::Stop()
{
  m_elm327.reset();
  m_speedKmh = -1;
  m_lastGyroTimestampNs = 0;
}

SourceState PhoneMotion::GetState() const
{
  return m_elm327 ? m_elm327->GetState() : SourceState::Disconnected;
}

void PhoneMotion::Calibrate()
{
  LOG(LINFO, ("Calibration"));
  m_calibrator.Reset();
  m_calibrationMoved = false;
  m_calibrationState = CalibrationState::Calibrating;
}

void PhoneMotion::OnSpeed(int speedKmh, int64_t timeMs)
{
  m_speedKmh = speedKmh;
  m_speedTimeMs = timeMs;
  if (speedKmh > 0)
    m_stoppedSinceMs.reset();
  else if (!m_stoppedSinceMs)
    m_stoppedSinceMs = timeMs;
  m_listener.OnSpeed(speedKmh, timeMs);
}

void PhoneMotion::OnAccel(Vec3 const & accel)
{
  m_accel = accel;
}

void PhoneMotion::OnGyro(int64_t timestampNs, Vec3 const & gyro)
{
  double const dt = m_lastGyroTimestampNs == 0 ? 0 : (timestampNs - m_lastGyroTimestampNs) / 1e9;
  m_lastGyroTimestampNs = timestampNs;
  if (dt <= 0 || dt > kMaxSensorDtSec || !m_accel)
    return;

  UpdateCalibration(gyro);
  double const yawDeltaDeg = IsCalibrated() ? GyroCalibrator::YawRateDeg(gyro, *m_bias, *m_up) * dt : 0;
  m_listener.OnMotion(yawDeltaDeg, dt, timestampNs);
}

bool PhoneMotion::IsCarMoving(int64_t nowMs) const
{
  return m_speedKmh > 0 && nowMs - m_speedTimeMs < kSpeedStaleMs;
}

void PhoneMotion::UpdateCalibration(Vec3 const & gyro)
{
  int64_t const now = m_clock.NowMs();
  if (m_calibrationState == CalibrationState::Calibrating)
  {
    m_calibrator.Add(gyro, *m_accel);
    // A car driving straight is as still for the phone as a standing one.
    if (IsCarMoving(now))
      m_calibrationMoved = true;
    if (m_calibrator.IsComplete())
    {
      if (m_calibrator.IsStill() && !m_calibrationMoved)
      {
        m_bias = m_calibrator.GetBias();
        m_up = m_calibrator.GetUp();
        m_calibrationState = CalibrationState::Done;
      }
      else
      {
        m_calibrationState = CalibrationState::FailedMoving;
      }
      LOG(LINFO, ("Calibration:", m_calibrationState));
    }
    return;
  }

  // Refresh the gyroscope bias on every stop, it changes with the temperature.
  bool const stopped = m_speedKmh == 0 && now - m_speedTimeMs < kSpeedStaleMs && m_stoppedSinceMs &&
                       now - *m_stoppedSinceMs > kAutoCalibrationDelayMs;
  if (!stopped)
  {
    m_autoCalibrator.Reset();
    m_previousAutoBias.reset();
    return;
  }
  m_autoCalibrator.Add(gyro, *m_accel);
  if (m_autoCalibrator.IsComplete())
  {
    if (m_autoCalibrator.IsStill())
      ApplyAutoCalibration(now - *m_stoppedSinceMs);
    else
      m_previousAutoBias.reset();
    m_autoCalibrator.Reset();
  }
}

void PhoneMotion::ApplyAutoCalibration(int64_t stoppedMs)
{
  Vec3 const up = m_autoCalibrator.GetUp();
  auto const previous = m_previousAutoBias;
  m_previousAutoBias = m_autoCalibrator.GetBias();
  if (!previous)
    return;
  auto const bias = GyroCalibrator::Confirm(*previous, *m_previousAutoBias, up);
  if (!bias)
    return;
  double const changeDeg = m_bias ? GyroCalibrator::YawRateDeg(*bias, *m_bias, up) : 0;
  if (!GyroCalibrator::IsBiasChangeAllowed(m_bias, *bias, up, stoppedMs))
  {
    LOG(LINFO, ("Automatic calibration ignored: the car turns slowly at", changeDeg, "deg/s"));
    return;
  }
  if (std::fabs(changeDeg) > 0.1)
    LOG(LINFO, ("Automatic calibration changes the yaw by", changeDeg, "deg/s"));
  m_bias = *bias;
  m_up = up;
  m_calibrationState = CalibrationState::Done;
  LOG(LDEBUG, ("Automatic calibration at a stop"));
}
}  // namespace nogps
