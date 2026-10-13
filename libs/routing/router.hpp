#pragma once

#include "routing/checkpoints.hpp"
#include "routing/road_graph.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_callbacks.hpp"

#include "kml/type_utils.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace routing
{

using CountryParentNameGetterFn = std::function<std::string(std::string const &)>;

// Guides with integer ids containing multiple tracks. One track consists of its points.
using GuidesTracks = std::map<kml::MarkGroupId, std::vector<kml::TrackGeometry>>;

class RoutesResult;

struct EdgeProj
{
  Edge m_edge;
  m2::PointD m_point;
};

/// A road a car may drive along, see IRouter::FindRoadsAlong().
struct RoadAlong
{
  // The closest point of the road.
  m2::PointD m_point;
  // The direction along the road the car goes.
  double m_angleRad = 0.0;
  // How far the road is, with its direction weighed in: the road of the least cost is the one the car is on.
  double m_cost = 0.0;
  // A one-way road going against the car, e.g. the other carriageway.
  bool m_wrongWay = false;
};

/// Routing engine type.
enum class RouterType
{
  // @TODO It's necessary to rename Vehicle value to Car.
  Vehicle = 0,  /// For Car routing.
  Pedestrian,   /// For A star pedestrian routing.
  Bicycle,      /// For A star bicycle routing.
  Transit,      /// For A star pedestrian + transit routing.
  Ruler,        /// For simple straight line router.
  Count         /// Number of router types.
};

std::string ToString(RouterType type);
RouterType FromString(std::string const & str);
std::string DebugPrint(RouterType type);

class IRouter
{
public:
  virtual ~IRouter() = default;

  /// Return unique name of a router implementation.
  virtual std::string GetName() const = 0;

  /// Clear all temporary buffers.
  virtual void ClearState() = 0;

  virtual void SetGuides(GuidesTracks && guides) = 0;

  /// Override this function with routing implementation.
  /// It will be called in separate thread and only one function will processed in same time.
  /// @warning please support Cancellable interface calls. You must stop processing when it is true.
  ///
  /// @param checkpoints start, finish and intermediate points
  /// @param startDirection start direction for routers with high cost of the turnarounds
  /// @param adjust adjust route to the previous one if possible
  /// @param needAlternatives compute alternative routes besides the main one
  /// @param delegate callback functions and cancellation flag
  /// @param result populated with one or more alternative routes (as RouteBase) when successful;
  ///               callers consume the active alternative via result.GetActive()
  /// @return ResultCode error code or NoError if at least one route was produced
  /// @see Cancellable
  virtual RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                          bool adjust, bool needAlternatives, RouterDelegate const & delegate,
                                          RoutesResult & result) = 0;

  virtual bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                           EdgeProj & proj) = 0;

  /// Collects the points in |rect| where roads cross or branch, i.e. where a car can turn.
  virtual void FindRoadCrossings(m2::RectD const & /* rect */, std::vector<m2::PointD> & /* crossings */) {}

  /// Finds the closest point of a road within |radiusM| of |point|, preferring a main road to a driveway or
  /// a yard road branching off it nearly as close. |angleRad| is the direction of the road there.
  virtual bool FindMainRoad(m2::PointD const & /* point */, double /* radiusM */, m2::PointD & /* projected */,
                            double & /* angleRad */)
  {
    return false;
  }

  /// Returns true if a roundabout is within |radiusM| of |point|.
  virtual bool IsRoundaboutNear(m2::PointD const & /* point */, double /* radiusM */) { return false; }

  /// How many meters of the distance to a road a degree of the difference between its direction and the
  /// direction of a car is worth when the road the car is on is chosen.
  static double constexpr kRoadMetersPerDeg = 0.2;

  /// Returns true if the end |roadPoint| of a road piece, the closest point of it, is behind a car at |point| going
  /// the |direction| way: the road ends behind the car, e.g. a driveway, and snapping to it would pull the car back on
  /// every snap. A point inside a piece is on a road going on, even a bit behind the car on a bend.
  static bool IsRoadBehind(m2::PointD const & point, m2::PointD const & roadPoint, m2::PointD const & direction);

  /// Finds the road a car at |point| going the |direction| way drives along: the closest one, but a road going
  /// the car's way is preferred to a closer one bending aside, and a road crossing the car's way is not taken.
  /// Without the direction it is the closest road. |angleRad| is the direction along the road the car goes.
  virtual bool FindRoadAlong(m2::PointD const & /* point */, m2::PointD const & /* direction */, double /* radiusM */,
                             m2::PointD & /* projected */, double & /* angleRad */)
  {
    return false;
  }

  /// Collects every road FindRoadAlong() chooses from, the closest place of a road each: where a road forks, the
  /// branch a car has taken is told by what the car does next.
  virtual void FindRoadsAlong(m2::PointD const & /* point */, m2::PointD const & /* direction */, double /* radiusM */,
                              std::vector<RoadAlong> & /* roads */)
  {}

  /// Swap the saved last-route state with the alternative's saved state. Called when the user
  /// picks an alternative variant so subsequent adjustments and full rebuilds (off-route rebuilds)
  /// keep the selected variant rather than the original primary. Default: no-op.
  virtual void SwapAltRouteToActive() {}
};

}  // namespace routing
