#include "drape_frontend/animation_utils.hpp"
#include "drape_frontend/animation_constants.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

// Zoom level before which animation can not be instant.
int const kInstantAnimationZoomLevel = 5;

namespace df
{

bool IsAnimationAllowed(double duration, ScreenBase const & screen)
{
  return duration > 0.0 && (duration < kMaxAnimationTimeSec || df::GetDrawTileScale(screen) < kInstantAnimationZoomLevel);
}

} // namespace df
