#pragma once

#include "indexer/ftypes_matcher.hpp"

namespace routing
{
struct LoadedPathSegment;

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

  TurnInfo() : m_ingoing(nullptr), m_outgoing(nullptr)
  {
  }

  TurnInfo(LoadedPathSegment const * ingoingSegment, LoadedPathSegment const * outgoingSegment)
      : m_ingoing(ingoingSegment), m_outgoing(outgoingSegment)
  {
  }

  bool IsSegmentsValid() const;
};

bool IsHighway(ftypes::HighwayClass hwClass, bool isLink);
bool IsSmallRoad(ftypes::HighwayClass hwClass);

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
T FindDirectionByAngle(vector<pair<double, T>> const & lowerBounds, double const angle);

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

}  // namespace turns
}  // namespace routing