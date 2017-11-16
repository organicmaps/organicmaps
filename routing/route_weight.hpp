#pragma once

#include "routing/base/astar_weight.hpp"

#include <iostream>
#include <limits>

namespace routing
{
class RouteWeight final
{
public:
  RouteWeight() = default;

  constexpr RouteWeight(double weight, int nonPassThroughCross)
    : m_weight(weight), m_nonPassThroughCross(nonPassThroughCross)
  {
  }

  static RouteWeight FromCrossMwmWeight(double weight)
  {
    return RouteWeight(weight, 0 /* nonPassThroughCross */);
  }

  double ToCrossMwmWeight() const;
  double GetWeight() const { return m_weight; }
  int GetNonPassThroughCross() const { return m_nonPassThroughCross; }

  bool operator<(RouteWeight const & rhs) const
  {
    if (m_nonPassThroughCross != rhs.m_nonPassThroughCross)
      return m_nonPassThroughCross < rhs.m_nonPassThroughCross;
    return m_weight < rhs.m_weight;
  }

  bool operator==(RouteWeight const & rhs) const { return !((*this) < rhs) && !(rhs < (*this)); }

  bool operator!=(RouteWeight const & rhs) const { return !((*this) == rhs); }

  bool operator>(RouteWeight const & rhs) const { return rhs < (*this); }

  bool operator>=(RouteWeight const & rhs) const { return !((*this) < rhs); }

  RouteWeight operator+(RouteWeight const & rhs) const
  {
    return RouteWeight(m_weight + rhs.m_weight, m_nonPassThroughCross + rhs.m_nonPassThroughCross);
  }

  RouteWeight operator-(RouteWeight const & rhs) const
  {
    return RouteWeight(m_weight - rhs.m_weight, m_nonPassThroughCross - rhs.m_nonPassThroughCross);
  }

  RouteWeight & operator+=(RouteWeight const & rhs)
  {
    m_weight += rhs.m_weight;
    m_nonPassThroughCross += rhs.m_nonPassThroughCross;
    return *this;
  }

  RouteWeight operator-() const { return RouteWeight(-m_weight, -m_nonPassThroughCross); }

private:
  // Regular weight (seconds).
  double m_weight = 0.0;
  // Number of pass-through/non-pass-through area border cross.
  int m_nonPassThroughCross = 0;
};

std::ostream & operator<<(std::ostream & os, RouteWeight const & routeWeight);

RouteWeight operator*(double lhs, RouteWeight const & rhs);

template <>
constexpr RouteWeight GetAStarWeightMax<RouteWeight>()
{
  return RouteWeight(std::numeric_limits<double>::max() /* weight */,
                     std::numeric_limits<int>::max() /* nonPassThroughCross */);
}

template <>
constexpr RouteWeight GetAStarWeightZero<RouteWeight>()
{
  return RouteWeight(0.0 /* weight */, 0 /* nonPassThroughCross */);
}

template <>
constexpr RouteWeight GetAStarWeightEpsilon<RouteWeight>()
{
  return RouteWeight(GetAStarWeightEpsilon<double>(), 0 /* nonPassThroughCross */);
}

}  // namespace routing
