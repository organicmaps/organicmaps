#pragma once

#include "routing/lanes/lane_info.hpp"

#include <cstddef>
#include <string>

namespace routing::turns::lanes
{
/// How many lane entries a UI is expected to render at most. OSM data legitimately reaches
/// ~50 lanes at toll plazas; after collapsing identical runs real junctions almost always fit.
size_t constexpr kMaxLanesToDisplay = 8;

struct CollapsedLanes
{
  LanesInfo lanes;
  /// Entries were dropped past the corresponding edge of the lane strip (physical road
  /// sides — lane order never mirrors with UI text direction), so the UI should hint that
  /// more lanes exist there.
  bool trimmedLeft = false;
  bool trimmedRight = false;

  bool operator==(CollapsedLanes const & rhs) const = default;
};

/// Prepares parsed lanes for display. Lanes that already fit kMaxLanesToDisplay are passed
/// through verbatim, one entry per physical lane. Otherwise:
/// 1. Runs of adjacent identical non-recommended lanes are merged into a single entry whose
///    similarLanesCount is the run length, preserving the exact lane picture.
/// 2. If more than kMaxLanesToDisplay entries remain, non-recommended entries are dropped —
///    outer edges first (flagged via trimmedLeft/trimmedRight), then interior ones — and
///    only when every remaining entry is recommended, entries are dropped from the right.
CollapsedLanes CollapseLanes(LanesInfo lanes);

std::string DebugPrint(CollapsedLanes const & collapsedLanes);
}  // namespace routing::turns::lanes
