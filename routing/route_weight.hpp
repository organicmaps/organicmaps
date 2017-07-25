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

  constexpr explicit RouteWeight(double weight) : m_weight(weight) {}

  double GetWeight() const { return m_weight; }

  bool operator<(RouteWeight const & rhs) const { return m_weight < rhs.m_weight; }

  bool operator==(RouteWeight const & rhs) const { return !((*this) < rhs) && !(rhs < (*this)); }

  bool operator!=(RouteWeight const & rhs) const { return !((*this) == rhs); }

  bool operator>(RouteWeight const & rhs) const { return rhs < (*this); }

  bool operator>=(RouteWeight const & rhs) const { return !((*this) < rhs); }

  RouteWeight operator+(RouteWeight const & rhs) const
  {
    return RouteWeight(m_weight + rhs.m_weight);
  }

  RouteWeight operator-(RouteWeight const & rhs) const
  {
    return RouteWeight(m_weight - rhs.m_weight);
  }

  RouteWeight & operator+=(RouteWeight const & rhs)
  {
    m_weight += rhs.m_weight;
    return *this;
  }

  RouteWeight operator-() const { return RouteWeight(-m_weight); }

private:
  double m_weight = 0.0;
};

std::ostream & operator<<(std::ostream & os, RouteWeight const & routeWeight);

RouteWeight operator*(double lhs, RouteWeight const & rhs);

template <>
constexpr RouteWeight GetAStarWeightMax<RouteWeight>()
{
  return RouteWeight(std::numeric_limits<double>::max());
}

template <>
constexpr RouteWeight GetAStarWeightZero<RouteWeight>()
{
  return RouteWeight(0.0);
}

template <>
constexpr RouteWeight GetAStarWeightEpsilon<RouteWeight>()
{
  return RouteWeight(GetAStarWeightEpsilon<double>());
}

}  // namespace routing
