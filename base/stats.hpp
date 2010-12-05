#pragma once
#include "assert.hpp"
#include "base.hpp"
#include "casts.hpp"
#include "pseudo_random.hpp"
#include "internal/message.hpp"
#include "../std/sstream.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace my
{

template <typename T> class NoopStats
{
public:
  NoopStats() {}
  inline void operator() (T const &) {}
  inline string GetStatsStr() const { return ""; }
};

template <typename T> class AverageStats
{
public:
  AverageStats() : m_Count(0), m_Sum(0) {}

  void operator() (T const & x)
  {
    ++m_Count;
    m_Sum += implicit_cast<double>(x);
  }

  string GetStatsStr() const
  {
    ostringstream out;
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
