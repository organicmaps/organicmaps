#pragma once

#include "search/result.hpp"

#include "geometry/any_rect2d.hpp"

namespace search_viewport
{
// Adjusts |viewport| to show the search results, keeping its rotation. Only the best matching results
// (with the minimum number of misprints) are taken into account:
// - one of them is already visible: nothing changes;
// - the nearest one is not far away: zooms out around the center (keeping the extents) to include it;
// - several localized ones: shows all of them at once;
// - a single result, or a set too spread out: shows the top ranked one at its own scale.
// Returns true if |viewport| was changed.
bool FitToResults(search::Results const & results, m2::AnyRectD & viewport);
}  // namespace search_viewport
