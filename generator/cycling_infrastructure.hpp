#pragma once

#include "indexer/cycling_infrastructure.hpp"

#include <optional>

struct OsmElement;

namespace generator
{
// Preserve explicit side/direction semantics; never invent a side from an
// unqualified aggregate category. Results follow the stored geometry direction.
std::optional<cycling::Sides> ReadCyclingSides(OsmElement const & element, bool reversed);
}  // namespace generator
