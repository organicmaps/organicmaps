#pragma once

#include "routing/loaded_path_segment.hpp"

#include "std/unique_ptr.hpp"

namespace routing
{
struct RoutingMapping;
struct RawPathData;
struct FeatureGraphNode;

namespace turns
{
using TSeg = OsrmMappingTypes::FtSeg;

// General constructor.
unique_ptr<LoadedPathSegment> LoadedPathSegmentFactory(RoutingMapping & mapping,
                                                       Index const & index,
                                                       RawPathData const & osrmPathSegment);
// Special constructor for side nodes. Splits OSRM node by information from the FeatureGraphNode.
unique_ptr<LoadedPathSegment> LoadedPathSegmentFactory(RoutingMapping & mapping, Index const & index,
                                                       RawPathData const & osrmPathSegment,
                                                       FeatureGraphNode const & startGraphNode,
                                                       FeatureGraphNode const & endGraphNode,
                                                       bool isStartNode, bool isEndNode);
}  // namespace routing
}  // namespace turns
