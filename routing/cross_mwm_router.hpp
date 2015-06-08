#pragma once

#include "osrm_engine.hpp"
#include "router.hpp"
#include "routing_mapping.h"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace routing
{
/*!
 * \brief The RoutePathCross struct contains information neaded to describe path inside single map.
 */
struct RoutePathCross
{
  FeatureGraphNode startNode; /**< start graph node representation */
  FeatureGraphNode finalNode; /**< end graph node representation */

  RoutePathCross(NodeID const startNode, NodeID const finalNode, string const & name)
      : startNode(startNode, true /* isStartNode */, name),
        finalNode(finalNode, false /* isStartNode*/, name)
  {
  }
};

using TCheckedPath = vector<RoutePathCross>;

/*!
 * \brief CalculateCrossMwmPath function for calculating path through several maps.
 * \param startGraphNodes The vector of starting routing graph nodes.
 * \param finalGraphNodes The vector of final routing graph nodes.
 * \param route Storage for the result records about crossing maps.
 * \param indexManager Manager for getting indexes of new countries.
 * \param RoutingVisualizerFn Debug visualization function.
 * \return NoError if the path exists, error code otherwise.
 */
IRouter::ResultCode CalculateCrossMwmPath(TRoutingNodes const & startGraphNodes,
                                          TRoutingNodes const & finalGraphNodes,
                                          RoutingIndexManager & indexManager,
                                          my::Cancellable const & cancellable,
                                          TRoutingVisualizerFn const & routingVisualizer,
                                          TCheckedPath & route);
}  // namespace routing
