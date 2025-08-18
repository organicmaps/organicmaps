#pragma once
#include "transit/transit_entities.hpp"

#include "geometry/point2d.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "3party/just_gtfs/just_gtfs.h"

namespace transit
{
// Projection point and mercator distance to it.
struct ProjectionToShape
{
  m2::PointD m_point;
  double m_dist;
};

enum class Direction
{
  Forward,
  Backward
};

/// \returns |stopPoint| projection to the track segment [|point1|, |point2|] and
/// distance from the |stopPoint| to its projection.
ProjectionToShape ProjectStopOnTrack(m2::PointD const & stopPoint, m2::PointD const & point1,
                                     m2::PointD const & point2);

/// \returns index of the nearest track point to the |point| and flag if it was inserted to the
/// shape. If this index doesn't match already existent points, the stop projection is inserted to
/// the |polyline| and the flag is set to true. New point should follow prevPoint in the direction
/// |direction|.
std::pair<size_t, bool> PrepareNearestPointOnTrack(m2::PointD const & point,
                                                   std::optional<m2::PointD> const & prevPoint, size_t prevIndex,
                                                   Direction direction, std::vector<m2::PointD> & polyline);

/// \returns true if we should not skip routes with this GTFS |routeType|.
bool IsRelevantType(gtfs::RouteType const & routeType);

/// \return string representation of the GTFS |routeType|.
std::string ToString(gtfs::RouteType const & routeType);

/// \return string representation of the extended GTFS |routeType|.
std::string ToStringExtendedType(gtfs::RouteType const & routeType);

/// \return stop times for trip with |tripId|.
gtfs::StopTimes GetStopTimesForTrip(gtfs::StopTimes const & allStopTimes, std::string const & tripId);

// Delete item from the |container| by its key.
template <class C, class K>
void DeleteIfExists(C & container, K const & key)
{
  auto it = container.find(key);
  if (it != container.end())
    container.erase(it);
}

template <class K>
void DeleteIfExists(std::vector<K> & container, K const & key)
{
  auto it = std::find(container.begin(), container.end(), key);
  if (it != container.end())
    container.erase(it);
}

// Delete items by keys in |keysForDel| from the |container|.
template <class C, class S>
void DeleteAllEntriesByIds(C & container, S const & keysForDel)
{
  for (auto const & key : keysForDel)
    DeleteIfExists(container, key);
}

// We have routes with multiple lines. Each line corresponds to the geometric polyline. Lines may
// be parallel in some segments. |LinePart| represents these operlapping segments for each line.
struct LinePart
{
  // Start and end indexes on polyline.
  LineSegment m_segment;
  // Parallel line ids to its start points on the segment.
  std::map<TransitId, m2::PointD> m_commonLines;
  // First coordinate of current line on the segment. It is used for determining if the line is
  // co-directional or reversed regarding the main line on the segment.
  m2::PointD m_firstPoint;
};

using LineParts = std::vector<LinePart>;

// Returns iterator to the line part with equivalent segment.
LineParts::iterator FindLinePart(LineParts & lineParts, LineSegment const & segment);

// Data required for finding parallel polyline segments and calculating offsets for each line on the
// segment.
struct LineSchemeData
{
  TransitId m_lineId = 0;
  std::string m_color;
  ShapeLink m_shapeLink;

  LineParts m_lineParts;
};

// Returns overlapping segments between two polylines.
std::pair<LineSegments, LineSegments> FindIntersections(std::vector<m2::PointD> const & line1,
                                                        std::vector<m2::PointD> const & line2);

// Finds item in |lineParts| equal to |segment| and updates it. If it doesn't exist it is added to
// the |lineParts|.
void UpdateLinePart(LineParts & lineParts, LineSegment const & segment, m2::PointD const & startPoint,
                    TransitId commonLineId, m2::PointD const & startPointParallel);

// Calculates start and end indexes of intersection of two segments: [start1, finish1] and [start2,
// finish2].
std::optional<LineSegment> GetIntersection(size_t start1, size_t finish1, size_t start2, size_t finish2);

// Calculates line order on segment based on two parameters: line index between all parallel lines,
// total parallel lines count. Line order must be symmetrical with respect to the сentral axis of
// the polyline.
int CalcSegmentOrder(size_t segIndex, size_t totalSegCount);

// Returns true if |stopIndex| doesn't equal size_t maximum value.
bool StopIndexIsSet(size_t stopIndex);

// Gets interval of stop indexes on the line with |lineStopIds| which belong to the region and
// its vicinity. |stopIdsInRegion| is set of all stop ids in the region.
std::pair<size_t, size_t> GetStopsRange(IdList const & lineStopIds, IdSet const & stopIdsInRegion);

// Returns indexes of points |p1| and |p2| on the |shape| polyline. If there are more then 1
// occurrences of |p1| or |p2| in |shape|, indexes with minimum distance are returned.
std::pair<size_t, size_t> FindPointsOnShape(std::vector<m2::PointD> const & shape, m2::PointD const & p1,
                                            m2::PointD const & p2);

// Returns indexes of first and last points of |segment| in the |shape| polyline. If |edgeShape|
// is not found returns pair of zeroes.
std::pair<size_t, size_t> FindSegmentOnShape(std::vector<m2::PointD> const & shape,
                                             std::vector<m2::PointD> const & segment);

// Returns reversed vector.
template <class T>
std::vector<T> GetReversed(std::vector<T> vec)
{
  std::reverse(vec.begin(), vec.end());
  return vec;
}
}  // namespace transit
