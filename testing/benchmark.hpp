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
      : m_RepeatCount(repeatCount), m_MaxSecondsToSucceed(maxSecondsToSucceed), m_Iteration(0)
    {
    }

    ~BenchmarkNTimes()
    {
      double const secondsElapsed = m_Timer.ElapsedSeconds();
      TEST_GREATER(m_RepeatCount, 0, ());
      TEST_LESS_OR_EQUAL(secondsElapsed, m_MaxSecondsToSucceed, (m_RepeatCount));
      std::cout << secondsElapsed << "s total";
      if (secondsElapsed > 0)
      {
        std::cout << ", " << static_cast<int>(m_RepeatCount / secondsElapsed) << "/s, ";
        /*
        if (secondsElapsed / m_RepeatCount * 1000 >= 10)
          std::cout << static_cast<int>(secondsElapsed / m_RepeatCount * 1000) << "ms each";
        else */ if (secondsElapsed / m_RepeatCount * 1000000 >= 10)
          std::cout << static_cast<int>(secondsElapsed / m_RepeatCount * 1000000) << "us each";
        else
          std::cout << static_cast<int>(secondsElapsed / m_RepeatCount * 1000000000) << "ns each";
      }
      std::cout << " ...";
    }

    int Iteration() const { return m_Iteration; }
    bool ContinueIterating() const { return m_Iteration < m_RepeatCount; }
    void NextIteration() { ++m_Iteration; }

  private:
    int const m_RepeatCount;
    double const m_MaxSecondsToSucceed;
    int m_Iteration;
    Timer m_Timer;
  };
}  // namespace base

#define BENCHMARK_TEST(name) \
    void Benchmark_##name();	\
        TestRegister g_BenchmarkRegister_##name("Benchmark::"#name, __FILE__, &Benchmark_##name); \
    void Benchmark_##name()

#define BENCHMARK_N_TIMES(times, maxTimeToSucceed) \
    for (::base::BenchmarkNTimes benchmark(times, maxTimeToSucceed); \
         benchmark.ContinueIterating(); benchmark.NextIteration())
