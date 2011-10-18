#include "api.hpp"

#include "../../std/iostream.hpp"
#include "../../std/numeric.hpp"
#include "../../std/algorithm.hpp"
#include "../../std/iomanip.hpp"

namespace bench
{

void Result::CalcMetrics()
{
  sort(m_time.begin(), m_time.end());

  m_max = m_time.back();
  m_med = m_time[m_time.size()/2];
  m_all = accumulate(m_time.begin(), m_time.end(), 0.0);
  m_avg = m_all / m_time.size();

  m_time.clear();
}

void AllResult::Print()
{
  m_reading.CalcMetrics();

  //              'all time',       'index time',                     'feature loading time'
  cout << fixed << setprecision(10);
  cout << "FRAME*1000[ median:" << m_reading.m_med * 1000 << " ";
  cout << "avg:" << m_reading.m_avg * 1000 << " ";
  cout << "max:" << m_reading.m_max * 1000 << " ] ";
  cout << "TOTAL[ idx:" << m_all - m_reading.m_all << " decoding:" << m_reading.m_all << " summ:" << m_all << " ]" << endl;
}

}
