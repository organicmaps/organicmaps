#pragma once

#include "base/logging.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>

namespace dp
{
static size_t constexpr kLastMeasurementsCount = 30;

class Profiler
{
public:
  static Profiler & Instance()
  {
    static Profiler profiler;
    return profiler;
  }

  void Measure(std::string const & name, uint32_t milliseconds)
  {
    auto & m = m_measurements[name];
    m.m_accumulator += milliseconds;
    m.m_lastMeasurements[m.m_count % kLastMeasurementsCount] = milliseconds;
    m.m_count++;
  }

  void Print()
  {
    LOG(LINFO, ("--- DRAPE PROFILER ---"));

    for (auto const & m : m_measurements)
    {
      float const avg = static_cast<float>(m.second.m_accumulator) / m.second.m_count;

      size_t const startIndex = m.second.m_count % kLastMeasurementsCount;
      std::ostringstream ss;
      ss << "[";
      for (size_t i = startIndex; i < std::min(kLastMeasurementsCount, m.second.m_count); ++i)
        ss << m.second.m_lastMeasurements[i] << ", ";
      for (size_t i = 0; i < startIndex; ++i)
        ss << m.second.m_lastMeasurements[i] << ", ";
      ss << ']';

      LOG(LINFO, (">", m.first, ": avg time =", avg, "count =", m.second.m_count, "last =", ss.str()));
    }
    LOG(LINFO, ("----------------------"));
  }

protected:
  struct Measurement
  {
    size_t m_count = 0;
    uint64_t m_accumulator = 0;
    std::array<uint32_t, kLastMeasurementsCount> m_lastMeasurements = {};
  };
  std::map<std::string, Measurement> m_measurements;
};

class ProfilerGuard
{
public:
  ProfilerGuard(std::string const & name) : m_name(name) { m_start = std::chrono::steady_clock::now(); }

  ~ProfilerGuard()
  {
    auto const dur = std::chrono::steady_clock::now() - m_start;
    auto ms = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count());
    if (!m_skipped)
      Profiler::Instance().Measure(m_name, ms);
  }

  void Skip() { m_skipped = true; }

private:
  std::string m_name;
  std::chrono::time_point<std::chrono::steady_clock> m_start;
  bool m_skipped = false;
};
}  // namespace dp
