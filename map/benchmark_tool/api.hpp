#pragma once

#include "std/vector.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"


namespace bench
{
  struct Result
  {
    vector<double> m_time;

  public:
    double m_all, m_max, m_avg, m_med;

  public:
    void Add(double t)
    {
      m_time.push_back(t);
    }
    void Add(Result const & r)
    {
      m_time.insert(m_time.end(), r.m_time.begin(), r.m_time.end());
    }

    void PrintAllTimes();
    void CalcMetrics();
  };

  class AllResult
  {
  public:
    Result m_reading;
    double m_all;

  public:
    AllResult() : m_all(0.0) {}

    void Add(double t) { m_all += t; }
    void Print();
  };

  /// @param[in] count number of times to run benchmark
  void RunFeaturesLoadingBenchmark(string const & file, pair<int, int> scaleR, AllResult & res);
}
