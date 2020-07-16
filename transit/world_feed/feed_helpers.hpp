#pragma once

#include "geometry/point2d.hpp"

#include <algorithm>
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

/// \returns |stopPoint| projection to the track segment [|point1|, |point2|] and
/// distance from the |stopPoint| to its projection.
ProjectionToShape ProjectStopOnTrack(m2::PointD const & stopPoint, m2::PointD const & point1,
                                     m2::PointD const & point2);

/// \returns index of the nearest track point to the |point| and flag if it was inserted to the
/// shape. If this index doesn't match already existent points, the stop projection is inserted to
/// the |polyline| and the flag is set to true.
std::pair<size_t, bool> PrepareNearestPointOnTrack(m2::PointD const & point,
                                                   std::optional<m2::PointD> const & prevPoint,
                                                   size_t startIndex,
                                                   std::vector<m2::PointD> & polyline);

/// \returns true if we should not skip routes with this GTFS |routeType|.
bool IsRelevantType(const gtfs::RouteType & routeType);

/// \return string representation of the GTFS |routeType|.
std::string ToString(gtfs::RouteType const & routeType);

/// \return string representation of the extended GTFS |routeType|.
std::string ToStringExtendedType(gtfs::RouteType const & routeType);

/// \return stop times for trip with |tripId|.
gtfs::StopTimes GetStopTimesForTrip(gtfs::StopTimes const & allStopTimes,
                                    std::string const & tripId);

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

inline double KmphToMps(double kmph) { return kmph * 1'000.0 / (60.0 * 60.0); }
inline double MpsToKmph(double mps) { return mps / 1'000.0 * 60.0 * 60.0; }
}  // namespace transit
