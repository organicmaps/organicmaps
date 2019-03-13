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

template <typename T, typename U,
          std::enable_if_t<!(std::is_signed<T>::value ^ std::is_signed<U>::value), int> = 0>
std::common_type_t<T, U> AbsDifference(T const a, U const b)
{
  return a >= b ? a - b : b - a;
}

bool PassesRestriction(Graph::Edge const & e, FunctionalRoadClass restriction, FormOfWay fow,
                       int frcThreshold, RoadInfoGetter & infoGetter);

/// \returns true if edge |e| conforms Lowest Functional Road Class to Next Point.
/// \note lfrcnp means Lowest Functional Road Class To Next LR-point.
/// Please see openlr documentation for details:
/// http://www.openlr.org/data/docs/whitepaper/1_0/OpenLR-Whitepaper_v1.0.pdf
bool ConformLfrcnp(Graph::Edge const & e, FunctionalRoadClass lfrcnp,
                   int frcThreshold, RoadInfoGetter & infoGetter);
}  // namespace openlr
