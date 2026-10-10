#include "map/nogps/accel_speed.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace nogps
{
namespace
{
// The longest lag of the car speed, see SpeedLag.
int64_t constexpr kMaxLagMs = 3000;
// A part of the time the sensor may be silent for.
double constexpr kMinCoverage = 0.8;
// The sums of the comparisons in Serialize().
size_t constexpr kSerializedValues = 21;

double Dot(AccelSpeed::Vec3 const & a, AccelSpeed::Vec3 const & b)
{
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
}  // namespace

void AccelSpeed::Reset()
{
  int64_t const lagMs = m_lagMs;
  *this = AccelSpeed(m_params);
  m_lagMs = lagMs;
}

void AccelSpeed::OnAccel(int64_t timeMs, Vec3 const & acceleration)
{
  bool const gap = m_lastAccelMs == 0 || timeMs < m_lastAccelMs || timeMs - m_lastAccelMs > m_params.m_maxAccelGapMs;
  if (gap)
    m_sliceCount = 0;
  // A sample tells about the time since the previous one.
  if (m_sliceCount == 0)
    m_sliceStartMs = gap ? timeMs : m_lastAccelMs;
  m_lastAccelMs = timeMs;
  for (size_t i = 0; i < 3; ++i)
    m_sliceSum[i] = (m_sliceCount == 0 ? 0 : m_sliceSum[i]) + acceleration[i];
  ++m_sliceCount;
  if (timeMs - m_sliceStartMs < m_params.m_sliceMs)
    return;

  Slice slice{timeMs, timeMs - m_sliceStartMs, {}};
  for (size_t i = 0; i < 3; ++i)
    slice.m_acceleration[i] = m_sliceSum[i] / m_sliceCount;
  m_slices.push_back(slice);
  m_sliceCount = 0;
  int64_t const keepMs = m_params.m_biasMs + m_params.m_carStepMs + kMaxLagMs + 2000;
  while (!m_slices.empty() && timeMs - m_slices.front().m_endMs > keepMs)
    m_slices.pop_front();
}

void AccelSpeed::OnCarSpeed(int64_t timeMs, double speedMps)
{
  if (!m_speeds.empty() && (timeMs < m_lastSpeedMs || timeMs - m_lastSpeedMs > m_params.m_maxSpeedGapMs))
    m_speeds.clear();
  m_lastSpeedMs = timeMs;
  // Only the changes are kept.
  if (m_speeds.empty() || m_speeds.back().m_speedMps != speedMps)
    m_speeds.push_back({timeMs, speedMps});
  int64_t const keepMs = m_params.m_biasMs + m_params.m_carStepMs + 2000;
  // The speed before the kept time is the one of the last sample before it.
  while (m_speeds.size() > 1 && timeMs - m_speeds[1].m_timeMs > keepMs)
    m_speeds.pop_front();

  if (m_learning && (timeMs < m_lastLearnMs || timeMs - m_lastLearnMs >= m_params.m_learnIntervalMs))
  {
    m_lastLearnMs = timeMs;
    Learn(timeMs);
  }
}

std::string AccelSpeed::Serialize() const
{
  std::string result;
  auto const add = [&result](double value)
  {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s%.9g", result.empty() ? "" : ";", value);
    result += buf;
  };
  add(m_n);
  add(m_sumY);
  add(m_sumYY);
  for (double v : m_sumX)
    add(v);
  for (double v : m_sumXY)
    add(v);
  for (double v : m_sumXX)
    add(v);
  for (double v : {m_checkN, m_checkA, m_checkY, m_checkAY, m_checkAA, m_checkYY})
    add(v);
  return result;
}

bool AccelSpeed::Deserialize(std::string const & data)
{
  std::vector<double> values;
  for (auto const item : strings::Tokenize(data, ";"))
  {
    double value;
    if (!strings::to_double(item, value) || !std::isfinite(value))
      return false;
    values.push_back(value);
  }
  if (values.size() != kSerializedValues || values[0] < 0)
    return false;
  size_t i = 0;
  m_n = values[i++];
  m_sumY = values[i++];
  m_sumYY = values[i++];
  for (double & v : m_sumX)
    v = values[i++];
  for (double & v : m_sumXY)
    v = values[i++];
  for (double & v : m_sumXX)
    v = values[i++];
  for (double * v : {&m_checkN, &m_checkA, &m_checkY, &m_checkAY, &m_checkAA, &m_checkYY})
    *v = values[i++];
  UpdateDirection();
  return true;
}

std::optional<double> AccelSpeed::CarSpeedAt(int64_t timeMs) const
{
  if (m_speeds.empty() || timeMs < m_speeds.front().m_timeMs || timeMs > m_lastSpeedMs)
    return {};
  auto it = std::upper_bound(m_speeds.begin(), m_speeds.end(), timeMs,
                             [](int64_t t, Speed const & speed) { return t < speed.m_timeMs; });
  return std::prev(it)->m_speedMps;
}

std::optional<AccelSpeed::Vec3> AccelSpeed::MeanAccel(int64_t fromMs, int64_t toMs) const
{
  Vec3 sum{};
  int64_t coveredMs = 0;
  for (auto it = m_slices.rbegin(); it != m_slices.rend() && it->m_endMs > fromMs; ++it)
  {
    int64_t const overlapMs = std::min(it->m_endMs, toMs) - std::max(it->m_endMs - it->m_durationMs, fromMs);
    if (overlapMs <= 0)
      continue;
    coveredMs += overlapMs;
    for (size_t i = 0; i < 3; ++i)
      sum[i] += it->m_acceleration[i] * overlapMs;
  }
  if (toMs <= fromMs || coveredMs < kMinCoverage * (toMs - fromMs))
    return {};
  for (double & v : sum)
    v /= coveredMs;
  return sum;
}

std::optional<double> AccelSpeed::GetBias(int64_t carTimeMs) const
{
  if (!m_hasDirection)
    return {};
  auto const now = CarSpeedAt(carTimeMs);
  if (!now)
    return {};
  // The car speed of now is the real one of the lag ago: its change over the time is the real acceleration
  // of the same time that ended the lag ago.
  for (int64_t ms = m_params.m_biasMs; ms >= m_params.m_minBiasMs; ms /= 2)
  {
    auto const before = CarSpeedAt(carTimeMs - ms);
    auto const accel = MeanAccel(carTimeMs - m_lagMs - ms, carTimeMs - m_lagMs);
    if (before && accel)
      return Dot(m_direction, *accel) - (*now - *before) * 1000 / ms;
  }
  return {};
}

void AccelSpeed::Learn(int64_t timeMs)
{
  auto const now = CarSpeedAt(timeMs);
  auto const before = CarSpeedAt(timeMs - m_params.m_carStepMs);
  auto const x = MeanAccel(timeMs - m_params.m_carStepMs - m_lagMs, timeMs - m_lagMs);
  if (!now || !before || !x)
    return;
  double const y = (*now - *before) * 1000 / m_params.m_carStepMs;
  if (std::fabs(y) < m_params.m_minCarAccelerationMps2)
    return;

  if (m_hasDirection)
  {
    double const a = Dot(m_direction, *x);
    double const keep = 1 - 1 / m_params.m_checkMemory;
    m_checkN = m_checkN * keep + 1;
    m_checkA = m_checkA * keep + a;
    m_checkY = m_checkY * keep + y;
    m_checkAY = m_checkAY * keep + a * y;
    m_checkAA = m_checkAA * keep + a * a;
    m_checkYY = m_checkYY * keep + y * y;
    if (m_checkN >= 2 * m_params.m_minCheckSamples &&
        (GetRecentCorrelation() < m_params.m_movedCorrelation || GetRecentScale() < m_params.m_movedScale))
    {
      Forget();
      return;
    }
  }

  double const keep = 1 - 1 / m_params.m_memory;
  m_n = m_n * keep + 1;
  m_sumY = m_sumY * keep + y;
  m_sumYY = m_sumYY * keep + y * y;
  size_t k = 0;
  for (size_t i = 0; i < 3; ++i)
  {
    m_sumX[i] = m_sumX[i] * keep + (*x)[i];
    m_sumXY[i] = m_sumXY[i] * keep + (*x)[i] * y;
    for (size_t j = i; j < 3; ++j, ++k)
      m_sumXX[k] = m_sumXX[k] * keep + (*x)[i] * (*x)[j];
  }
  UpdateDirection();
}

void AccelSpeed::Forget()
{
  m_n = m_sumY = m_sumYY = 0;
  m_sumX.fill(0);
  m_sumXY.fill(0);
  m_sumXX.fill(0);
  m_checkN = m_checkA = m_checkY = m_checkAY = m_checkAA = m_checkYY = 0;
  m_hasDirection = false;
  m_correlation = 0;
}

void AccelSpeed::UpdateDirection()
{
  m_hasDirection = false;
  m_correlation = 0;
  if (m_n < 2)
    return;
  double const meanY = m_sumY / m_n;
  double const varY = m_sumYY / m_n - meanY * meanY;
  Vec3 meanX;
  Vec3 cov;
  double norm = 0;
  for (size_t i = 0; i < 3; ++i)
  {
    meanX[i] = m_sumX[i] / m_n;
    cov[i] = m_sumXY[i] / m_n - meanX[i] * meanY;
    norm += cov[i] * cov[i];
  }
  norm = std::sqrt(norm);
  if (norm < 1e-9 || varY < 1e-9)
    return;
  // The sensor follows the car along the direction its acceleration changes together with the car speed in.
  for (size_t i = 0; i < 3; ++i)
    m_direction[i] = cov[i] / norm;
  double varAlong = 0;
  size_t k = 0;
  for (size_t i = 0; i < 3; ++i)
  {
    for (size_t j = i; j < 3; ++j, ++k)
    {
      double const c = m_sumXX[k] / m_n - meanX[i] * meanX[j];
      varAlong += (i == j ? 1 : 2) * c * m_direction[i] * m_direction[j];
    }
  }
  if (varAlong < 1e-9)
    return;
  m_hasDirection = true;
  m_correlation = norm / std::sqrt(varAlong * varY);
}

double AccelSpeed::GetRecentCorrelation() const
{
  if (m_checkN < 2)
    return 0;
  double const meanA = m_checkA / m_checkN;
  double const meanY = m_checkY / m_checkN;
  double const varA = m_checkAA / m_checkN - meanA * meanA;
  double const varY = m_checkYY / m_checkN - meanY * meanY;
  if (varA < 1e-9 || varY < 1e-9)
    return 0;
  return (m_checkAY / m_checkN - meanA * meanY) / std::sqrt(varA * varY);
}

double AccelSpeed::GetRecentScale() const
{
  if (m_checkN < 2)
    return 0;
  double const meanA = m_checkA / m_checkN;
  double const meanY = m_checkY / m_checkN;
  double const varY = m_checkYY / m_checkN - meanY * meanY;
  if (varY < 1e-9)
    return 0;
  return (m_checkAY / m_checkN - meanA * meanY) / varY;
}

std::optional<double> AccelSpeed::GetAccelAlong(int64_t fromMs, int64_t toMs) const
{
  if (!IsUsable())
    return {};
  auto const accel = MeanAccel(fromMs, toMs);
  if (!accel)
    return {};
  return Dot(m_direction, *accel);
}

std::optional<AccelSpeed::Vec3> AccelSpeed::GetDirection() const
{
  if (!m_hasDirection)
    return {};
  return m_direction;
}

bool AccelSpeed::IsUsable() const
{
  if (!m_hasDirection || m_n < m_params.m_minSamples || m_correlation < m_params.m_minCorrelation ||
      m_checkN < m_params.m_minCheckSamples || GetRecentCorrelation() < m_params.m_minRecentCorrelation)
  {
    return false;
  }
  double const scale = GetRecentScale();
  return scale >= m_params.m_minRecentScale && scale <= m_params.m_maxRecentScale;
}

std::optional<double> AccelSpeed::GetGain(int64_t nowMs) const
{
  if (!IsUsable() || nowMs < m_lastSpeedMs || nowMs - m_lastSpeedMs > m_params.m_maxSpeedGapMs)
    return {};
  auto const bias = GetBias(m_lastSpeedMs);
  auto const accel = MeanAccel(nowMs - m_lagMs, nowMs);
  if (!bias || !accel)
    return {};
  double const lagSec = m_lagMs / 1000.0;
  double const limit = m_params.m_maxAccelerationMps2 * lagSec;
  return std::clamp((Dot(m_direction, *accel) - *bias) * lagSec, -limit, limit);
}
}  // namespace nogps
