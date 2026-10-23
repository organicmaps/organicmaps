#pragma once

#include "testing/testing.hpp"
#include "testing/testregister.hpp"

#include "base/timer.hpp"

#include <iostream>

namespace base
{
// Timings depend on the host and its load, so report them without pass/fail limits.
class BenchmarkNTimes
{
public:
  explicit BenchmarkNTimes(int repeatCount) : m_repeatCount(repeatCount), m_iteration(0)
  {
    TEST_GREATER(m_repeatCount, 0, ());
  }

  ~BenchmarkNTimes()
  {
    double const secondsElapsed = m_timer.ElapsedSeconds();
    std::cout << secondsElapsed << "s total";
    if (secondsElapsed > 0)
    {
      std::cout << ", " << static_cast<int>(m_repeatCount / secondsElapsed) << "/s, ";
      /*
        if (secondsElapsed / m_repeatCount * 1000 >= 10)
          std::cout << static_cast<int>(secondsElapsed / m_repeatCount * 1000) << "ms each";
        else */
      if (secondsElapsed / m_repeatCount * 1000000 >= 10)
        std::cout << static_cast<int>(secondsElapsed / m_repeatCount * 1000000) << "us each";
      else
        std::cout << static_cast<int>(secondsElapsed / m_repeatCount * 1000000000) << "ns each";
    }
    std::cout << " ...";
  }

  int Iteration() const { return m_iteration; }
  bool ContinueIterating() const { return m_iteration < m_repeatCount; }
  void NextIteration() { ++m_iteration; }

private:
  int const m_repeatCount;
  int m_iteration;
  Timer m_timer;
};
}  // namespace base

#define BENCHMARK_TEST(name)                                                                 \
  void Benchmark_##name();                                                                   \
  TestRegister g_BenchmarkRegister_##name("Benchmark::" #name, __FILE__, &Benchmark_##name); \
  void Benchmark_##name()

#define BENCHMARK_N_TIMES(times)                                                                           \
  for (::base::BenchmarkNTimes benchmark(times); benchmark.ContinueIterating(); benchmark.NextIteration())
