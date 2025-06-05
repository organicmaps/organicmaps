#pragma once

#include "openlr/graph.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/score_types.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <string>
#include <type_traits>

namespace openlr
{
class RoadInfoGetter;

// This class is used to get points for further bearing calculations.
class BearingPointsSelector
{
public:
  BearingPointsSelector(uint32_t bearDistM, bool isLastPoint);

  m2::PointD GetStartPoint(Graph::Edge const & e) const;
  m2::PointD GetEndPoint(Graph::Edge const & e, double distanceM) const;

private:
  double m_bearDistM;
  bool m_isLastPoint;
};

bool PointsAreClose(m2::PointD const & p1, m2::PointD const & p2);

double EdgeLength(Graph::Edge const & e);

bool EdgesAreAlmostEqual(Graph::Edge const & e1, Graph::Edge const & e2);

// TODO(mgsergio): Remove when unused.
std::string LogAs2GisPath(Graph::EdgeVector const & path);
std::string LogAs2GisPath(Graph::Edge const & e);

template <typename T, typename U, std::enable_if_t<!(std::is_signed<T>::value ^ std::is_signed<U>::value), int> = 0>
std::common_type_t<T, U> AbsDifference(T const a, U const b)
{
  return a >= b ? a - b : b - a;
}

bool PassesRestriction(Graph::Edge const & e, FunctionalRoadClass restriction, FormOfWay formOfWay, int frcThreshold,
                       RoadInfoGetter & infoGetter);
/// \returns true if |e| conforms |functionalRoadClass| and |formOfWay| and false otherwise.
/// \note If the method returns true |score| should be considered next.
bool PassesRestrictionV3(Graph::Edge const & e, FunctionalRoadClass functionalRoadClass, FormOfWay formOfWay,
                         RoadInfoGetter & infoGetter, Score & score);

/// \returns true if edge |e| conforms Lowest Functional Road Class to Next Point.
/// \note frc means Functional Road Class. Please see openlr documentation for details:
/// http://www.openlr.org/data/docs/whitepaper/1_0/OpenLR-Whitepaper_v1.0.pdf
bool ConformLfrcnp(Graph::Edge const & e, FunctionalRoadClass lowestFrcToNextPoint, int frcThreshold,
                   RoadInfoGetter & infoGetter);
bool ConformLfrcnpV3(Graph::Edge const & e, FunctionalRoadClass lowestFrcToNextPoint, RoadInfoGetter & infoGetter);

size_t IntersectionLen(Graph::EdgeVector a, Graph::EdgeVector b);

bool SuffixEqualsPrefix(Graph::EdgeVector const & a, Graph::EdgeVector const & b, size_t len);

// Returns a length of the longest suffix of |a| that matches any prefix of |b|.
// Neither |a| nor |b| can contain several repetitions of any edge.
// Returns -1 if |a| intersection |b| is not equal to some suffix of |a| and some prefix of |b|.
int32_t PathOverlappingLen(Graph::EdgeVector const & a, Graph::EdgeVector const & b);

m2::PointD PointAtSegmentM(m2::PointD const & p1, m2::PointD const & p2, double const distanceM);
}  // namespace openlr
