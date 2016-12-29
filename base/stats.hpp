#pragma once
#include "base/base.hpp"

#include <sstream>
#include <string>


namespace my
{

template <typename T> class NoopStats
{
public:
  NoopStats() {}
  inline void operator() (T const &) {}
  inline std::string GetStatsStr() const { return ""; }
};

template <typename T> class AverageStats
{
public:
  AverageStats() : m_Count(0), m_Sum(0) {}

  void operator() (T const & x)
  {
    ++m_Count;
    m_Sum += x;
  }

  string GetStatsStr() const
  {
    std::ostringstream out;
    out << "N: " << m_Count;
    if (m_Count > 0)
      out << " Av: " << m_Sum / m_Count;
    return out.str();
  }

  double GetAverage() const
  {
    return m_Count ? m_Sum / m_Count : 0.0;
  }

  uint32_t GetCount() const
  {
    return m_Count;
  }

private:
  uint32_t m_Count;
  double m_Sum;
};

}
