#pragma once

#include "routing/base/astar_weight.hpp"

#include "base/math.hpp"

#include <iostream>
#include <limits>

namespace routing
{
class RouteWeight final
{
public:
  RouteWeight() = default;

  explicit constexpr RouteWeight(double weight) : m_weight(weight) {}

  constexpr RouteWeight(double weight, int nonPassThroughCross, double transitTime)
    : m_weight(weight), m_nonPassThroughCross(nonPassThroughCross), m_transitTime(transitTime)
  {
  }

  static RouteWeight FromCrossMwmWeight(double weight) { return RouteWeight(weight); }

  double ToCrossMwmWeight() const;
  double GetWeight() const { return m_weight; }
  int GetNonPassThroughCross() const { return m_nonPassThroughCross; }
  double GetTransitTime() const { return m_transitTime; }

  bool operator<(RouteWeight const & rhs) const
  {
    if (m_nonPassThroughCross != rhs.m_nonPassThroughCross)
      return m_nonPassThroughCross < rhs.m_nonPassThroughCross;
    if (m_weight != rhs.m_weight)
      return m_weight < rhs.m_weight;
    // Preffer bigger transit time if total weights are same.
    return m_transitTime > rhs.m_transitTime;
  }

  bool operator==(RouteWeight const & rhs) const { return !((*this) < rhs) && !(rhs < (*this)); }

  bool operator!=(RouteWeight const & rhs) const { return !((*this) == rhs); }

  bool operator>(RouteWeight const & rhs) const { return rhs < (*this); }

  bool operator>=(RouteWeight const & rhs) const { return !((*this) < rhs); }

  bool operator<=(RouteWeight const & rhs) const { return rhs >= (*this); }

  RouteWeight operator+(RouteWeight const & rhs) const
  {
    return RouteWeight(m_weight + rhs.m_weight, m_nonPassThroughCross + rhs.m_nonPassThroughCross,
                       m_transitTime + rhs.m_transitTime);
  }

  RouteWeight operator-(RouteWeight const & rhs) const
  {
    return RouteWeight(m_weight - rhs.m_weight, m_nonPassThroughCross - rhs.m_nonPassThroughCross,
                       m_transitTime - rhs.m_transitTime);
  }

  RouteWeight & operator+=(RouteWeight const & rhs)
  {
    m_weight += rhs.m_weight;
    m_nonPassThroughCross += rhs.m_nonPassThroughCross;
    m_transitTime += rhs.m_transitTime;
    return *this;
  }

  RouteWeight operator-() const
  {
    return RouteWeight(-m_weight, -m_nonPassThroughCross, -m_transitTime);
  }

  bool IsAlmostEqualForTests(RouteWeight const & rhs, double epsilon)
  {
    return m_nonPassThroughCross == rhs.m_nonPassThroughCross &&
           my::AlmostEqualAbs(m_weight, rhs.m_weight, epsilon) &&
           my::AlmostEqualAbs(m_transitTime, rhs.m_transitTime, epsilon);
  }

private:
  // Regular weight (seconds).
  double m_weight = 0.0;
  // Number of pass-through/non-pass-through area border cross.
  int m_nonPassThroughCross = 0;
  // Transit time. It's already included in |m_weight| (m_transitTime <= m_weight).
  double m_transitTime = 0.0;
};

std::ostream & operator<<(std::ostream & os, RouteWeight const & routeWeight);

RouteWeight operator*(double lhs, RouteWeight const & rhs);

template <>
constexpr RouteWeight GetAStarWeightMax<RouteWeight>()
{
  return RouteWeight(std::numeric_limits<double>::max() /* weight */,
                     std::numeric_limits<int>::max() /* nonPassThroughCross */,
                     std::numeric_limits<int>::max() /* transitTime */);
}

template <>
constexpr RouteWeight GetAStarWeightZero<RouteWeight>()
{
  return RouteWeight(0.0 /* weight */, 0 /* nonPassThroughCross */, 0.0 /* transitTime */);
}

template <>
constexpr RouteWeight GetAStarWeightEpsilon<RouteWeight>()
{
  return RouteWeight(GetAStarWeightEpsilon<double>(), 0 /* nonPassThroughCross */,
                     0.0 /* transitTime */);
}

}  // namespace routing
