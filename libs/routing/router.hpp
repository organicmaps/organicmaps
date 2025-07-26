#pragma once

#include "routing/checkpoints.hpp"
#include "routing/road_graph.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_callbacks.hpp"

#include "kml/type_utils.hpp"

#include "geometry/point_with_altitude.hpp"
#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace routing
{

using TCountryFileFn = std::function<std::string(m2::PointD const &)>;
using CountryParentNameGetterFn = std::function<std::string(std::string const &)>;

// Guides with integer ids containing multiple tracks. One track consists of its points.
using GuidesTracks = std::map<kml::MarkGroupId, std::vector<std::vector<geometry::PointWithAltitude>>>;

class Route;

struct EdgeProj
{
  Edge m_edge;
  m2::PointD m_point;
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
  virtual ~IRouter() {}

  /// Return unique name of a router implementation.
  virtual std::string GetName() const = 0;

  /// Clear all temporary buffers.
  virtual void ClearState() {}

  virtual void SetGuides(GuidesTracks && guides) = 0;

  /// Override this function with routing implementation.
  /// It will be called in separate thread and only one function will processed in same time.
  /// @warning please support Cancellable interface calls. You must stop processing when it is true.
  ///
  /// @param startPoint point to start routing
  /// @param startDirection start direction for routers with high cost of the turnarounds
  /// @param finalPoint target point for route
  /// @param adjust adjust route to the previous one if possible
  /// @param delegate callback functions and cancellation flag
  /// @param route result route
  /// @return ResultCode error code or NoError if route was initialised
  /// @see Cancellable
  virtual RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                          bool adjust, RouterDelegate const & delegate, Route & route) = 0;

  virtual bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                           EdgeProj & proj) = 0;
};

}  // namespace routing
