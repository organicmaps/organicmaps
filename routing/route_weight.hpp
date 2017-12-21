#pragma once

#include "routing/base/astar_weight.hpp"

#include "base/math.hpp"

#include <cstdint>
#include <iostream>
#include <limits>

namespace routing
{
class RouteWeight final
{
public:
  RouteWeight() = default;

  explicit constexpr RouteWeight(double weight) : m_weight(weight) {}

  constexpr RouteWeight(double weight, int32_t nonPassThroughCross, int32_t numAccessChanges,
                        double transitTime)
    : m_weight(weight)
    , m_nonPassThroughCross(nonPassThroughCross)
    , m_numAccessChanges(numAccessChanges)
    , m_transitTime(transitTime)
  {
  }

  static RouteWeight FromCrossMwmWeight(double weight) { return RouteWeight(weight); }

  double ToCrossMwmWeight() const;
  double GetWeight() const { return m_weight; }
  int32_t GetNonPassThroughCross() const { return m_nonPassThroughCross; }
  int32_t GetNumAccessChanges() const { return m_numAccessChanges; }
  double GetTransitTime() const { return m_transitTime; }

  bool operator<(RouteWeight const & rhs) const
  {
    if (m_nonPassThroughCross != rhs.m_nonPassThroughCross)
      return m_nonPassThroughCross < rhs.m_nonPassThroughCross;
    // We compare m_numAccessChanges after m_nonPassThroughCross because we can have multiple nodes
    // with access tags on the way from the area with limited access and no access tags on the ways
    // inside this area. So we probably need to make access restriction less strict than pass
    // through restrictions e.g. allow to cross access={private, destination} and build the route
    // with the least possible number of such crosses or introduce some maximal number of
    // access={private, destination} crosses.
    if (m_numAccessChanges != rhs.m_numAccessChanges)
      return m_numAccessChanges < rhs.m_numAccessChanges;
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

  RouteWeight operator+(RouteWeight const & rhs) const;

  RouteWeight operator-(RouteWeight const & rhs) const;

  RouteWeight & operator+=(RouteWeight const & rhs);

  RouteWeight operator-() const
  {
    ASSERT_NOT_EQUAL(m_nonPassThroughCross, std::numeric_limits<int32_t>::min(), ());
    ASSERT_NOT_EQUAL(m_numAccessChanges, std::numeric_limits<int32_t>::min(), ());
    return RouteWeight(-m_weight, -m_nonPassThroughCross, -m_numAccessChanges, -m_transitTime);
  }

  bool IsAlmostEqualForTests(RouteWeight const & rhs, double epsilon)
  {
    return m_nonPassThroughCross == rhs.m_nonPassThroughCross &&
           m_numAccessChanges == rhs.m_numAccessChanges &&
           my::AlmostEqualAbs(m_weight, rhs.m_weight, epsilon) &&
           my::AlmostEqualAbs(m_transitTime, rhs.m_transitTime, epsilon);
  }

private:
  // Note: consider smaller types for m_nonPassThroughCross and m_numAccessChanges
  // in case of adding new fields to RouteWeight to reduce RouteWeight size.

  // Regular weight (seconds).
  double m_weight = 0.0;
  // Number of pass-through/non-pass-through area border crosses.
  int32_t m_nonPassThroughCross = 0;
  // Number of access=yes/access={private,destination} area border crosses.
  int32_t m_numAccessChanges = 0;
  // Transit time. It's already included in |m_weight| (m_transitTime <= m_weight).
  double m_transitTime = 0.0;
};

std::ostream & operator<<(std::ostream & os, RouteWeight const & routeWeight);

RouteWeight operator*(double lhs, RouteWeight const & rhs);

template <>
constexpr RouteWeight GetAStarWeightMax<RouteWeight>()
{
  return RouteWeight(std::numeric_limits<double>::max() /* weight */,
                     std::numeric_limits<int32_t>::max() /* nonPassThroughCross */,
                     std::numeric_limits<int32_t>::max() /* numAccessChanges */,
                     0 /* transitTime */);  // operator< prefers bigger transit time
}

template <>
constexpr RouteWeight GetAStarWeightZero<RouteWeight>()
{
  return RouteWeight(0.0 /* weight */, 0 /* nonPassThroughCross */, 0 /* numAccessChanges */,
                     0.0 /* transitTime */);
}

template <>
constexpr RouteWeight GetAStarWeightEpsilon<RouteWeight>()
{
  return RouteWeight(GetAStarWeightEpsilon<double>(), 0 /* nonPassThroughCross */,
                     0 /* numAccessChanges */, 0.0 /* transitTime */);
}

}  // namespace routing
