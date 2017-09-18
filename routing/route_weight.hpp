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

  constexpr RouteWeight(double weight, int nontransitCross)
    : m_weight(weight), m_nontransitCross(nontransitCross)
  {
  }

  static RouteWeight FromCrossMwmWeight(double weight)
  {
    return RouteWeight(weight, 0 /* nontransitCross */);
  }

  double ToCrossMwmWeight() const;
  double GetWeight() const { return m_weight; }
  int GetNontransitCross() const { return m_nontransitCross; }

  bool operator<(RouteWeight const & rhs) const
  {
    if (m_nontransitCross != rhs.m_nontransitCross)
      return m_nontransitCross < rhs.m_nontransitCross;
    return m_weight < rhs.m_weight;
  }

  bool operator==(RouteWeight const & rhs) const { return !((*this) < rhs) && !(rhs < (*this)); }

  bool operator!=(RouteWeight const & rhs) const { return !((*this) == rhs); }

  bool operator>(RouteWeight const & rhs) const { return rhs < (*this); }

  bool operator>=(RouteWeight const & rhs) const { return !((*this) < rhs); }

  RouteWeight operator+(RouteWeight const & rhs) const
  {
    return RouteWeight(m_weight + rhs.m_weight, m_nontransitCross + rhs.m_nontransitCross);
  }

  RouteWeight operator-(RouteWeight const & rhs) const
  {
    return RouteWeight(m_weight - rhs.m_weight, m_nontransitCross - rhs.m_nontransitCross);
  }

  RouteWeight & operator+=(RouteWeight const & rhs)
  {
    m_weight += rhs.m_weight;
    m_nontransitCross += rhs.m_nontransitCross;
    return *this;
  }

  RouteWeight operator-() const { return RouteWeight(-m_weight, -m_nontransitCross); }

private:
  // Regular weight (seconds).
  double m_weight = 0.0;
  // Number of transit/nontransit area border cross.
  int m_nontransitCross = 0;
};

std::ostream & operator<<(std::ostream & os, RouteWeight const & routeWeight);

RouteWeight operator*(double lhs, RouteWeight const & rhs);

template <>
constexpr RouteWeight GetAStarWeightMax<RouteWeight>()
{
  return RouteWeight(std::numeric_limits<double>::max() /* weight */,
                     std::numeric_limits<int>::max() /* nontransitCross */);
}

template <>
constexpr RouteWeight GetAStarWeightZero<RouteWeight>()
{
  return RouteWeight(0.0 /* weight */, 0 /* nontransitCross */);
}

template <>
constexpr RouteWeight GetAStarWeightEpsilon<RouteWeight>()
{
  return RouteWeight(GetAStarWeightEpsilon<double>(), 0 /* nontransitCross */);
}

}  // namespace routing
