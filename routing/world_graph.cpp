#include "routing/world_graph.hpp"

namespace routing
{
using namespace std;

string DebugPrint(WorldGraph::Mode mode)
{
  switch (mode)
  {
  case WorldGraph::Mode::LeapsOnly: return "LeapsOnly";
  case WorldGraph::Mode::LeapsIfPossible: return "LeapsIfPossible";
  case WorldGraph::Mode::NoLeaps: return "NoLeaps";
  case WorldGraph::Mode::SingleMwm: return "SingleMwm";
  }
  ASSERT(false, ("Unknown mode:", static_cast<size_t>(mode)));
  return "Unknown mode";
}
}  // namespace routing
