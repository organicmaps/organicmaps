#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"

#include <cstdint>
#include <functional>

struct OsmElement;

namespace ftype
{
/// Get the types, name and layer for feature with the tree of tags.
void GetNameAndType(OsmElement * p, FeatureParams & params,
                    std::function<bool(uint32_t)> filterType = feature::TypeIsUseful);
}
