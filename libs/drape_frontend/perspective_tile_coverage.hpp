#pragma once

#include "drape_frontend/tile_utils.hpp"
#include "geometry/screenbase.hpp"

namespace df
{
// Vector maps without building extrusion. The near-car band retains full map detail.
TTilesCollection SelectPerspectiveTiles(ScreenBase const & screen, int renderZoom, double extension,
                                        m2::PointD const & anchor, TTilesCollection const & previous);
}  // namespace df
