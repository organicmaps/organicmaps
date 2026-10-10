#include "map/nogps/accel_log.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace nogps
{
namespace
{
int64_t constexpr kAccelSliceNs = 1'000'000'000LL / AccelLog::kSlices;
}  // namespace

std::optional<std::string> AccelLog::OnSample(int64_t timestampNs, double x, double y, double z, double jolt)
{
  std::optional<std::string> line;
  // A break of the sensor starts everything anew.
  if (m_count > 0 && (timestampNs < m_sliceStartNs || timestampNs - m_sliceStartNs > 2 * kAccelSliceNs))
    Reset();
  if (m_count > 0 && timestampNs - m_sliceStartNs >= kAccelSliceNs)
  {
    EndSlice();
    if (m_slices == kSlices)
    {
      line = std::move(m_line);
      m_line.clear();
      m_slices = 0;
    }
  }
  if (m_count == 0)
    m_sliceStartNs = timestampNs;
  std::array<double, 3> const sample = {x, y, z};
  m_jolt = m_count == 0 ? jolt : std::max(m_jolt, jolt);
  for (size_t i = 0; i < 3; ++i)
  {
    m_sum[i] += sample[i];
    m_min[i] = m_count == 0 ? sample[i] : std::min(m_min[i], sample[i]);
    m_max[i] = m_count == 0 ? sample[i] : std::max(m_max[i], sample[i]);
  }
  ++m_count;
  return line;
}

void AccelLog::Reset()
{
  m_line.clear();
  m_slices = 0;
  ClearSlice();
}

void AccelLog::ClearSlice()
{
  m_count = 0;
  m_sum.fill(0);
}

void AccelLog::EndSlice()
{
  double jolt = m_jolt;
  for (size_t i = 0; i < 3; ++i)
  {
    double const mean = m_sum[i] / m_count;
    jolt = std::max(jolt, std::max(m_max[i] - mean, mean - m_min[i]));
  }
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%s%.2f,%.2f,%.2f,%.1f", m_slices > 0 ? " " : "", m_sum[0] / m_count,
                m_sum[1] / m_count, m_sum[2] / m_count, jolt);
  m_line += buf;
  ++m_slices;
  ClearSlice();
}

std::optional<std::string> GyroLog::OnSample(int64_t timestampNs, double yawDeltaDeg, int speedKmh)
{
  std::optional<std::string> line;
  // A break of the sensor starts the second anew, the total goes on.
  if (m_hasSlice && (timestampNs < m_sliceStartNs || timestampNs - m_sliceStartNs > 2 * kAccelSliceNs))
  {
    m_line.clear();
    m_speeds.clear();
    m_slices = 0;
    m_rotationDeg = 0;
    m_hasSlice = false;
  }
  if (m_hasSlice && timestampNs - m_sliceStartNs >= kAccelSliceNs)
  {
    char buf[32];
    std::snprintf(buf, sizeof(buf), " %.2f", m_sliceDeg);
    m_line += buf;
    m_speeds += " " + std::to_string(m_sliceSpeedKmh);
    m_hasSlice = false;
    if (++m_slices == kSlices)
    {
      std::snprintf(buf, sizeof(buf), "%.1f", m_totalDeg);
      line = buf + m_line + m_speeds;
      m_lineRotationDeg = m_rotationDeg;
      m_line.clear();
      m_speeds.clear();
      m_slices = 0;
      m_rotationDeg = 0;
    }
  }
  if (!m_hasSlice)
  {
    m_hasSlice = true;
    m_sliceStartNs = timestampNs;
    m_sliceDeg = 0;
  }
  m_sliceDeg += yawDeltaDeg;
  m_sliceSpeedKmh = speedKmh;
  m_rotationDeg += std::fabs(yawDeltaDeg);
  m_totalDeg += yawDeltaDeg;
  return line;
}

void GyroLog::Reset()
{
  *this = {};
}
}  // namespace nogps
