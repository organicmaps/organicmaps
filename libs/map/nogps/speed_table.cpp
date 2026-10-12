#include "map/nogps/speed_table.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace nogps
{
namespace
{
int SpeedRange(double carSpeedKmh)
{
  return std::min(SpeedTable::kRanges - 1, static_cast<int>(carSpeedKmh / SpeedTable::kRangeKmh));
}
}  // namespace

void SpeedTable::OnSample(double carSpeedKmh, double gpsSpeedMps, double dtSec, double accelerationMps2)
{
  if (carSpeedKmh < kMinSpeedKmh || dtSec <= 0 || dtSec > kMaxSampleDtSec ||
      std::fabs(accelerationMps2) > kMaxAccelerationMps2)
  {
    return;
  }
  int const i = SpeedRange(carSpeedKmh);
  m_carM[i] += carSpeedKmh / 3.6 * dtSec;
  m_gpsM[i] += gpsSpeedMps * dtSec;
  if (m_carM[i] > kMaxDistanceM)
  {
    m_carM[i] /= 2;
    m_gpsM[i] /= 2;
  }
}

double SpeedTable::ScaleOf(int i) const
{
  return std::clamp(m_gpsM[i] / m_carM[i], kMinScale, kMaxScale);
}

std::optional<double> SpeedTable::GetScale(double carSpeedKmh) const
{
  int const i = SpeedRange(carSpeedKmh);
  if (IsKnown(i))
    return ScaleOf(i);
  int lower = i - 1;
  while (lower >= 0 && !IsKnown(lower))
    --lower;
  int upper = i + 1;
  while (upper < kRanges && !IsKnown(upper))
    ++upper;
  if (lower < 0 && upper >= kRanges)
    return {};
  if (lower < 0)
    return ScaleOf(upper);
  if (upper >= kRanges)
    return ScaleOf(lower);
  double const t = static_cast<double>(i - lower) / (upper - lower);
  return ScaleOf(lower) + (ScaleOf(upper) - ScaleOf(lower)) * t;
}

int SpeedTable::GetKnownRanges() const
{
  int count = 0;
  for (int i = 0; i < kRanges; ++i)
    if (IsKnown(i))
      ++count;
  return count;
}

void SpeedTable::Clear()
{
  m_carM.fill(0);
  m_gpsM.fill(0);
}

std::string SpeedTable::Serialize() const
{
  std::string result;
  for (int i = 0; i < kRanges; ++i)
  {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s%.0f:%.0f", i > 0 ? ";" : "", m_carM[i], m_gpsM[i]);
    result += buf;
  }
  return result;
}

void SpeedTable::Deserialize(std::string_view table)
{
  Clear();
  auto const ranges = strings::Tokenize(table, ";");
  if (ranges.size() != kRanges)
    return;
  for (int i = 0; i < kRanges; ++i)
  {
    auto const distances = strings::Tokenize(ranges[i], ":");
    if (distances.size() != 2 || !strings::to_double(distances[0], m_carM[i]) ||
        !strings::to_double(distances[1], m_gpsM[i]))
    {
      Clear();
      return;
    }
  }
}

std::string SpeedTable::ToString() const
{
  std::string result;
  for (int i = 0; i < kRanges; ++i)
  {
    if (!IsKnown(i))
      continue;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s%d:%.3f", result.empty() ? "" : " ", i * kRangeKmh, ScaleOf(i));
    result += buf;
  }
  return result;
}
}  // namespace nogps
