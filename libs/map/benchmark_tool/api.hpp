#pragma once

#include <string>
#include <utility>
#include <vector>

namespace bench
{
class Result
{
public:
  void Add(double t) { m_time.push_back(t); }

  void Add(Result const & r) { m_time.insert(m_time.end(), r.m_time.begin(), r.m_time.end()); }

  void PrintAllTimes();
  void CalcMetrics();

  double m_all = 0.0;
  double m_max = 0.0;
  double m_avg = 0.0;
  double m_med = 0.0;

private:
  std::vector<double> m_time;
};

class AllResult
{
public:
  AllResult() = default;

  void Add(double t) { m_all += t; }
  void Print();

  Result m_reading;
  double m_all = 0.0;
};

/// @param[in] count number of times to run benchmark
void RunFeaturesLoadingBenchmark(std::string filePath, std::pair<int, int> scaleR, AllResult & res);
}  // namespace bench
