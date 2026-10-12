#include "map/nogps/gyro_calibrator.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>

namespace nogps
{
void GyroCalibrator::Reset()
{
  m_gyroSum.fill(0);
  m_gyroSqSum.fill(0);
  m_accelSum.fill(0);
  m_accelSqSum.fill(0);
  m_count = 0;
}

void GyroCalibrator::Add(Vec3 const & gyro, Vec3 const & accel)
{
  for (size_t i = 0; i < 3; ++i)
  {
    m_gyroSum[i] += gyro[i];
    m_gyroSqSum[i] += gyro[i] * gyro[i];
    m_accelSum[i] += accel[i];
    m_accelSqSum[i] += accel[i] * accel[i];
  }
  ++m_count;
}

int GyroCalibrator::GetProgressPercent() const
{
  return std::min(100, m_count * 100 / kRequiredSamples);
}

bool GyroCalibrator::IsStill() const
{
  for (size_t i = 0; i < 3; ++i)
  {
    if (Std(m_gyroSum[i], m_gyroSqSum[i]) > kMaxStillGyroStdRadS)
      return false;
    if (Std(m_accelSum[i], m_accelSqSum[i]) > kMaxStillAccelStdMps2)
      return false;
  }
  return true;
}

Vec3 GyroCalibrator::GetBias() const
{
  return {m_gyroSum[0] / m_count, m_gyroSum[1] / m_count, m_gyroSum[2] / m_count};
}

Vec3 GyroCalibrator::GetUp() const
{
  double const x = m_accelSum[0] / m_count;
  double const y = m_accelSum[1] / m_count;
  double const z = m_accelSum[2] / m_count;
  double const norm = std::sqrt(x * x + y * y + z * z);
  return {x / norm, y / norm, z / norm};
}

bool GyroCalibrator::IsBiasChangeAllowed(std::optional<Vec3> const & oldBias, Vec3 const & newBias, Vec3 const & up,
                                         int64_t stoppedMs)
{
  return !oldBias || stoppedMs >= kLongStopMs || std::fabs(YawRateDeg(newBias, *oldBias, up)) <= kMaxBiasChangeDegS;
}

std::optional<Vec3> GyroCalibrator::Confirm(Vec3 const & previousBias, Vec3 const & bias, Vec3 const & up)
{
  if (std::fabs(YawRateDeg(bias, previousBias, up)) > kMaxCalibrationsDiffDegS)
    return {};
  return Vec3{(previousBias[0] + bias[0]) / 2, (previousBias[1] + bias[1]) / 2, (previousBias[2] + bias[2]) / 2};
}

double GyroCalibrator::YawRateDeg(Vec3 const & gyro, Vec3 const & bias, Vec3 const & up)
{
  double dot = 0;
  for (size_t i = 0; i < 3; ++i)
    dot += (gyro[i] - bias[i]) * up[i];
  return -math::RadToDeg(dot);
}

double GyroCalibrator::Std(double sum, double sqSum) const
{
  double const mean = sum / m_count;
  return std::sqrt(std::max(0.0, sqSum / m_count - mean * mean));
}
}  // namespace nogps
