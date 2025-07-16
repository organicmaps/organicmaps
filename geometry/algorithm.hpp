#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <array>
#include <type_traits>
#include <utility>
#include <vector>

namespace m2
{
// This class is used to calculate a point on a polyline
// such as that the paths (not the distance) from that point
// to both ends of a polyline have the same length.
class CalculatePolyLineCenter
{
public:
  CalculatePolyLineCenter() : m_length(0.0) {}
  void operator()(PointD const & pt);

  PointD GetResult() const;

private:
  struct Value
  {
    Value(PointD const & p, double l) : m_p(p), m_len(l) {}
    bool operator<(Value const & r) const { return (m_len < r.m_len); }
    PointD m_p;
    double m_len;
  };

  std::vector<Value> m_poly;
  double m_length;
};

// This class is used to calculate a point such as that
// it lays on the figure triangle that is the closest to
// figure bounding box center.
class CalculatePointOnSurface
{
public:
  CalculatePointOnSurface(RectD const & rect);

  void operator()(PointD const & p1, PointD const & p2, PointD const & p3);
  PointD GetResult() const { return m_center; }

private:
  PointD m_rectCenter;
  PointD m_center;
  double m_squareDistanceToApproximate;
};

// Calculates the smallest rect that includes given geometry.
class CalculateBoundingBox
{
public:
  void operator()(PointD const & p);
  RectD GetResult() const { return m_boundingBox; }

private:
  RectD m_boundingBox;
};

namespace impl
{
template <typename TCalculator, typename TIterator>
m2::PointD ApplyPointOnSurfaceCalculator(TIterator begin, TIterator end, TCalculator && calc)
{
  std::array<m2::PointD, 3> triangle;
  while (begin != end)
  {
    for (auto i = 0; i < 3; ++i)
    {
      // Cannot use ASSERT_NOT_EQUAL, due to absence of an approbriate DebugPrint.
      ASSERT(begin != end, ("Not enough points to calculate point on surface"));
      triangle[i] = *begin++;
    }
    calc(triangle[0], triangle[1], triangle[2]);
  }
  return calc.GetResult();
}

template <typename TCalculator, typename TIterator>
auto ApplyCalculator(TIterator begin, TIterator end, TCalculator && calc) -> decltype(calc.GetResult())
{
  for (; begin != end; ++begin)
    calc(*begin);
  return calc.GetResult();
}

template <typename TCalculator, typename TIterator>
auto SelectImplementation(TIterator begin, TIterator end, TCalculator && calc, std::true_type const &)
    -> decltype(calc.GetResult())
{
  return impl::ApplyPointOnSurfaceCalculator(begin, end, std::forward<TCalculator>(calc));
}

template <typename TCalculator, typename TIterator>
auto SelectImplementation(TIterator begin, TIterator end, TCalculator && calc, std::false_type const &)
    -> decltype(calc.GetResult())
{
  return impl::ApplyCalculator(begin, end, std::forward<TCalculator>(calc));
}
}  // namespace impl

template <typename TCalculator, typename TIterator>
auto ApplyCalculator(TIterator begin, TIterator end, TCalculator && calc) -> decltype(calc.GetResult())
{
  return impl::SelectImplementation(begin, end, std::forward<TCalculator>(calc),
                                    std::is_same<CalculatePointOnSurface, std::remove_reference_t<TCalculator>>());
}

template <typename TCalculator, typename TCollection>
auto ApplyCalculator(TCollection && collection, TCalculator && calc) -> decltype(calc.GetResult())
{
  return ApplyCalculator(std::begin(collection), std::end(collection), std::forward<TCalculator>(calc));
}
}  // namespace m2
