#pragma once

#include "geometry/rect2d.hpp"

#include <string>

namespace feature
{
struct GenerateInfo;
}

namespace generator
{
// Invalidate provenance before every raw country generation, including opt-out runs.
void ResetWaterGeometry(std::string const & temporaryDirectory);

// Requires intermediate features generated with bounding-box water affiliation,
// complete source ways/relations and an explicit coverage declaration.
bool BuildWaterGeometrySection(feature::GenerateInfo const & info, std::string const & name,
                               m2::RectD const & sourceBounds);
}  // namespace generator
