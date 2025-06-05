#include "map/benchmark_tool/api.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>

using namespace std;

namespace bench
{
void Result::PrintAllTimes()
{
  sort(m_time.begin(), m_time.end());
  copy(m_time.begin(), m_time.end(), std::ostream_iterator<double>(cout, ", "));
  cout << endl;
}

void Result::CalcMetrics()
{
  if (!m_time.empty())
  {
    sort(m_time.begin(), m_time.end());

    m_max = m_time.back();
    m_med = m_time[m_time.size() / 2];
    m_all = accumulate(m_time.begin(), m_time.end(), 0.0);
    m_avg = m_all / m_time.size();

    m_time.clear();
  }
  else
    m_all = -1.0;
}

void AllResult::Print()
{
  // m_reading.PrintAllTimes();
  m_reading.CalcMetrics();

  if (m_all < 0.0)
    cout << "No frames" << endl;
  else
  {
    cout << fixed << setprecision(10);
    size_t const count = 1000;
    cout << "FRAME*1000[ median:" << m_reading.m_med * count << " avg:" << m_reading.m_avg * count
         << " max:" << m_reading.m_max * count << " ] ";
    cout << "TOTAL[ idx:" << m_all - m_reading.m_all << " decoding:" << m_reading.m_all << " summ:" << m_all << " ]"
         << endl;
  }
}
}  // namespace bench
