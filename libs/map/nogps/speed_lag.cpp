#include "map/nogps/speed_lag.hpp"

#include "base/string_utils.hpp"

#include <cmath>
#include <cstdio>

namespace nogps
{
namespace
{
int64_t constexpr kSpeedLagHistoryMs = static_cast<int64_t>(SpeedLag::kMaxLagSec * 1000) + 2000;
}  // namespace

void SpeedLag::OnCarSpeed(int64_t timeMs, double speedMps)
{
  m_car.push_back({timeMs, speedMps});
  while (m_car.size() > 1 && timeMs - m_car.front().m_timeMs > kSpeedLagHistoryMs)
    m_car.pop_front();
  while (!m_gps.empty() && timeMs - m_gps.front().m_timeMs >= kMaxLagSec * 1000)
  {
    Sample const gps = m_gps.front();
    m_gps.pop_front();
    Compare(gps);
  }
}

void SpeedLag::OnGpsSpeed(int64_t timeMs, double speedMps)
{
  int64_t const dtMs = timeMs - m_lastGpsMs;
  double const acceleration = dtMs > 0 ? (speedMps - m_lastGpsSpeedMps) * 1000 / dtMs : 0;
  bool const first = m_lastGpsMs == 0 || dtMs <= 0 || dtMs > kMaxGpsGapMs;
  m_lastGpsMs = timeMs;
  m_lastGpsSpeedMps = speedMps;
  if (!first && std::fabs(acceleration) >= kMinAccelerationMps2)
    m_gps.push_back({timeMs, speedMps});
}

void SpeedLag::Compare(Sample const & gps)
{
  // The car speed must be known all the time around the sample.
  if (m_car.empty() || m_car.front().m_timeMs > gps.m_timeMs)
    return;
  std::array<double, kSteps> errors;
  for (int i = 0; i < kSteps; ++i)
  {
    auto const car = CarSpeedAt(gps.m_timeMs + i * kStepSec * 1000);
    if (!car)
      return;
    errors[i] = (*car - gps.m_speedMps) * (*car - gps.m_speedMps);
  }
  for (int i = 0; i < kSteps; ++i)
    m_errors[i] += errors[i];
  if (++m_samples > kMaxSamples)
  {
    m_samples /= 2;
    for (auto & error : m_errors)
      error /= 2;
  }
}

std::optional<double> SpeedLag::CarSpeedAt(double timeMs) const
{
  std::optional<double> speed;
  double previousMs = 0;
  for (auto const & sample : m_car)
  {
    if (sample.m_timeMs > timeMs)
      break;
    speed = sample.m_speedMps;
    previousMs = sample.m_timeMs;
  }
  if (speed && timeMs - previousMs > kMaxGpsGapMs)
    return {};
  return speed;
}

double SpeedLag::Get() const
{
  if (!IsMeasured())
    return kDefaultLagSec;
  int best = 0;
  for (int i = 1; i < kSteps; ++i)
    if (m_errors[i] < m_errors[best])
      best = i;
  if (best == 0 || best == kSteps - 1)
    return best * kStepSec;
  // The bottom of the parabola through the best lag and its neighbours.
  double const left = m_errors[best - 1];
  double const right = m_errors[best + 1];
  double const curve = left - 2 * m_errors[best] + right;
  double const shift = curve > 0 ? 0.5 * (left - right) / curve : 0;
  return (best + shift) * kStepSec;
}

void SpeedLag::Clear()
{
  m_samples = 0;
  m_errors.fill(0);
  m_gps.clear();
}

std::string SpeedLag::Serialize() const
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.0f", m_samples);
  std::string result = buf;
  for (double const error : m_errors)
  {
    std::snprintf(buf, sizeof(buf), ";%.1f", error);
    result += buf;
  }
  return result;
}

void SpeedLag::Deserialize(std::string_view saved)
{
  Clear();
  auto const parts = strings::Tokenize(saved, ";");
  if (parts.size() != kSteps + 1)
    return;
  bool ok = strings::to_double(parts[0], m_samples);
  for (int i = 0; ok && i < kSteps; ++i)
    ok = strings::to_double(parts[i + 1], m_errors[i]);
  if (!ok)
    Clear();
}
}  // namespace nogps
