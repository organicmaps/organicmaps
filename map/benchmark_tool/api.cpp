#include "api.hpp"

#include "../../std/iostream.hpp"
#include "../../std/numeric.hpp"
#include "../../std/algorithm.hpp"


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
  cout << "all: " << m_all << ' ' << m_all - m_reading.m_all << ' ' << m_reading.m_all << endl;
  cout << "med: " << m_reading.m_med << endl;
  cout << "avg: " << m_reading.m_avg << endl;
  cout << "max: " << m_reading.m_max << endl;
}

}
