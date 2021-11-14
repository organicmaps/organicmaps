#include "junction_visitor.hpp"

#include "routing/joint_segment.hpp"
#include "routing/route_weight.hpp"

#include "base/logging.hpp"

namespace routing
{

#ifdef DEBUG
void DebugRoutingState(JointSegment const & vertex, std::optional<JointSegment> const & parent,
                       RouteWeight const & heuristic, RouteWeight const & distance)
{
  // 1. Dump current processing vertex.
//  std::cout << DebugPrint(vertex);
//  std::cout << std::setprecision(8) << "; H = " << heuristic << "; D = " << distance;

  // 2. Dump parent vertex.
//  std::cout << "; P = " << (parent ? DebugPrint(*parent) : std::string("NO"));

//  std::cout << std::endl;

  // 3. Set breakpoint on a specific vertex.
//  if (vertex.GetMwmId() == 706 && vertex.GetFeatureId() == 147648 &&
//      vertex.GetEndSegmentId() == 75)
//  {
//    int noop = 0;
//  }
}
#endif

} // namespace routing
