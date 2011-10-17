#include "api.hpp"

#include "../../std/iostream.hpp"

#include "../../base/start_mem_debug.hpp"


namespace bench
{

void AllResult::Print(bool perFrame) const
{
  size_t count = m_all.m_count;
  if (perFrame)
    count *= m_reading.m_count;

  // 'all time', 'index time', 'feature loading time'
  cout << m_all.m_time / count << ' ' <<
          (m_all.m_time - m_reading.m_time) / count << ' ' <<
          m_reading.m_time / count << endl;
}

}
