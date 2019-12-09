#pragma once

namespace routing
{
enum class WorldGraphMode
{
  LeapsOnly,  // Mode for building a cross mwm route containing only leaps. In case of start and
              // finish they (start and finish) will be connected with all transition segments of
              // their mwm with leap (fake) edges.
  NoLeaps,    // Mode for building route and getting outgoing/ingoing edges without leaps at all.
  SingleMwm,  // Mode for building route and getting outgoing/ingoing edges within mwm source
              // segment belongs to.
  Joints,     // Mode for building route with jumps between Joints.
  JointSingleMwm,  // Like |SingleMwm|, but in |Joints| mode.

  Undefined  // Default mode, until initialization.
};
}  // namespace routing
