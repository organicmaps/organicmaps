#pragma once

#include "geometry/point_with_altitude.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "routing/loaded_path_segment.hpp"

namespace routing
{
namespace turns
{
enum class CarDirection;
enum class PedestrianDirection;
/*!
 * \brief The TurnInfo structure is a representation of a junction.
 * It has ingoing and outgoing edges and method to check if these edges are valid.
 */
struct TurnInfo
{
  LoadedPathSegment const * m_ingoing;
  LoadedPathSegment const * m_outgoing;

  TurnInfo() : m_ingoing(nullptr), m_outgoing(nullptr) {}

  TurnInfo(LoadedPathSegment const * ingoingSegment, LoadedPathSegment const * outgoingSegment)
    : m_ingoing(ingoingSegment)
    , m_outgoing(outgoingSegment)
  {}

  bool IsSegmentsValid() const;
};

bool IsHighway(ftypes::HighwayClass hwClass, bool isLink);
bool IsSmallRoad(ftypes::HighwayClass hwClass);

// Min difference between HighwayClasses of the route segment and alternative turn segment
// to ignore this alternative candidate.
int constexpr kMinHighwayClassDiff = -2;

// Min difference between HighwayClasses of the route segment and alternative turn segment
// to ignore this alternative candidate (when alternative road is service).
int constexpr kMinHighwayClassDiffForService = -1;

/// * \returns difference between highway classes.
/// * It should be considered that bigger roads have smaller road class.
int CalcDiffRoadClasses(ftypes::HighwayClass const left, ftypes::HighwayClass const right);

/*!
 * \brief Converts a turn angle into a turn direction.
 * \note lowerBounds is a table of pairs: an angle and a direction.
 * lowerBounds shall be sorted by the first parameter (angle) from big angles to small angles.
 * These angles should be measured in degrees and should belong to the range [-180; 180].
 * The second paramer (angle) shall belong to the range [-180; 180] and is measured in degrees.
 */
template <class T>
T FindDirectionByAngle(std::vector<std::pair<double, T>> const & lowerBounds, double const angle);

CarDirection InvertDirection(CarDirection const dir);

/*!
 * \param angle is an angle of a turn. It belongs to a range [-180, 180].
 * \return correct direction if the route follows along the rightmost possible way.
 */
CarDirection RightmostDirection(double const angle);
CarDirection LeftmostDirection(double const angle);

/*!
 * \param angle is an angle of a turn. It belongs to a range [-180, 180].
 * \return correct direction if the route follows not along one of two outermost ways
 * or if there is only one possible way.
 */
CarDirection IntermediateDirection(double const angle);

PedestrianDirection IntermediateDirectionPedestrian(double const angle);

double CalcEstimatedTimeToPass(double const distanceMeters, ftypes::HighwayClass const highwayClass);

/// \returns true if |path| is loop connected to the PartOfReal segments.
bool PathIsFakeLoop(std::vector<geometry::PointWithAltitude> const & path);

// Returns distance in meters between |junctions[start]| and |junctions[end]|.
double CalcRouteDistanceM(std::vector<geometry::PointWithAltitude> const & junctions, uint32_t start, uint32_t end);

/*!
 * \brief Index of point in TUnpackedPathSegments. |m_segmentIndex| is a zero based index in vector
 * TUnpackedPathSegments. |m_pathIndex| in a zero based index in LoadedPathSegment::m_path.
 */
struct RoutePointIndex
{
  std::size_t m_segmentIndex = 0;
  std::size_t m_pathIndex = 0;

  bool operator==(RoutePointIndex const & index) const;
};

std::string DebugPrint(RoutePointIndex const & index);

RoutePointIndex GetFirstOutgoingPointIndex(size_t const outgoingSegmentIndex);

RoutePointIndex GetLastIngoingPointIndex(TUnpackedPathSegments const & segments, size_t const outgoingSegmentIndex);

double CalcOneSegmentTurnAngle(TurnInfo const & turnInfo);
double CalcPathTurnAngle(LoadedPathSegment const & segment, size_t const pathIndex);

m2::PointD GetPointByIndex(TUnpackedPathSegments const & segments, RoutePointIndex const & index);

}  // namespace turns
}  // namespace routing
