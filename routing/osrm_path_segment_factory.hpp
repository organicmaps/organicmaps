#pragma once

#include "routing/loaded_path_segment.hpp"

namespace routing
{
struct FeatureGraphNode;
struct RawPathData;
struct RoutingMapping;

// General constructor.
void OsrmPathSegmentFactory(RoutingMapping & mapping, Index const & index,
                            RawPathData const & osrmPathSegment, LoadedPathSegment & loadedPathSegment);
// Special constructor for side nodes. Splits OSRM node by information from the FeatureGraphNode.
void OsrmPathSegmentFactory(RoutingMapping & mapping, Index const & index, RawPathData const & osrmPathSegment,
                            FeatureGraphNode const & startGraphNode, FeatureGraphNode const & endGraphNode,
                            bool isStartNode, bool isEndNode, LoadedPathSegment & loadedPathSegment);
}  // namespace routing
