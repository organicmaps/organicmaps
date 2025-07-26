#pragma once

#include "testing/testing.hpp"
#include "testing/testregister.hpp"

#include "base/timer.hpp"

#include <iostream>

namespace base
{
class BenchmarkNTimes
{
public:
  BenchmarkNTimes(int repeatCount, double maxSecondsToSucceed)
    : m_repeatCount(repeatCount)
    , m_maxSecondsToSucceed(maxSecondsToSucceed)
    , m_iteration(0)
  {}

  ~BenchmarkNTimes()
  {
    double const secondsElapsed = m_timer.ElapsedSeconds();
    TEST_GREATER(m_repeatCount, 0, ());
    TEST_LESS_OR_EQUAL(secondsElapsed, m_maxSecondsToSucceed, (m_repeatCount));
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
  double const m_maxSecondsToSucceed;
  int m_iteration;
  Timer m_timer;
};
}  // namespace base

#define BENCHMARK_TEST(name)                                                                 \
  void Benchmark_##name();                                                                   \
  TestRegister g_BenchmarkRegister_##name("Benchmark::" #name, __FILE__, &Benchmark_##name); \
  void Benchmark_##name()

#define BENCHMARK_N_TIMES(times, maxTimeToSucceed)                                                \
  for (::base::BenchmarkNTimes benchmark(times, maxTimeToSucceed); benchmark.ContinueIterating(); \
       benchmark.NextIteration())
