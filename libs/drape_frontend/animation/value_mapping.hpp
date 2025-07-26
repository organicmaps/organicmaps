#pragma once

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"

#include <utility>

namespace df
{

template <typename TValue>
class ValueMapping
{
  /// double = interpolation point [0.0, 1.0]
  /// TValue = output value
  using TRangePoint = std::pair<double, TValue>;
  using TRangeVector = buffer_vector<TRangePoint, 8>;
  using TRangeIter = typename TRangeVector::const_iterator;

public:
  ValueMapping() = default;

  void AddRangePoint(double t, TValue const & v)
  {
#ifdef DEBUG
    if (!m_ranges.empty())
      ASSERT(m_ranges.back().first < t, ());
#endif
    m_ranges.emplace_back(t, v);
  }

  TValue GetValue(double t)
  {
    TRangePoint startPoint, endPoint;
    GetRangePoints(t, startPoint, endPoint);

    double normT = t - startPoint.first;
    double rangeT = endPoint.first - startPoint.first;
    double rangeV = endPoint.second - startPoint.second;

    return startPoint.second + rangeV * normT / rangeT;
  }

private:
  void GetRangePoints(double t, TRangePoint & startPoint, TRangePoint & endPoint)
  {
    ASSERT(t >= 0.0 && t <= 1.0, ());
    ASSERT(m_ranges.size() > 1, ());

    TRangeIter startIter = m_ranges.begin();
    if (t < startIter->first)
    {
      startPoint.first = 0.0;
      startPoint.second = TValue();
      endPoint = *startIter;
      return;
    }

    TRangeIter endIter = startIter + 1;
    while (t > endIter->first)
    {
      endIter++;
      ASSERT(m_ranges.end() != endIter, ());
    }

    ASSERT(startIter->first <= t && t <= endIter->first, ());
    startPoint = *startIter;
    endPoint = *endIter;
  }

private:
  TRangeVector m_ranges;
};

}  // namespace df
