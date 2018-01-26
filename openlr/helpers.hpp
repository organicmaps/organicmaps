#pragma once

#include "openlr/graph.hpp"
#include "openlr/openlr_model.hpp"

#include "geometry/point2d.hpp"

#include <type_traits>

namespace openlr
{
class RoadInfoGetter;

bool PointsAreClose(m2::PointD const & p1, m2::PointD const & p2);

double EdgeLength(Graph::Edge const & e);

bool EdgesAreAlmostEqual(Graph::Edge const & e1, Graph::Edge const & e2);

// TODO(mgsergio): Remove when unused.
std::string LogAs2GisPath(Graph::EdgeVector const & path);
std::string LogAs2GisPath(Graph::Edge const & e);

template <
    typename T, typename U,
    typename std::enable_if<!(std::is_signed<T>::value ^ std::is_signed<U>::value), int>::type = 0>
typename std::common_type<T, U>::type AbsDifference(T const a, U const b)
{
  return a >= b ? a - b : b - a;
}

bool PassesRestriction(Graph::Edge const & e, FunctionalRoadClass const restriction,
                       int const frcThreshold, RoadInfoGetter & infoGetter);
}  // namespace openlr
