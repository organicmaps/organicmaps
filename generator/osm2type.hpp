#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"

#include <cstdint>
#include <functional>
#include <optional>

struct OsmElement;

namespace ftype
{
using TypesFilterFnT = std::function<bool(uint32_t)>;
using CalculateOriginFnT = std::function<std::optional<m2::PointD>(OsmElement const * p)>;

/// Get the types, name and layer for feature with the tree of tags.
/// @param[in]  calcOrg Not empty in generator, maybe empty in unit tests.
void GetNameAndType(OsmElement * p, FeatureBuilderParams & params,
                    TypesFilterFnT const & filterType = &feature::IsUsefulType,
                    CalculateOriginFnT const & calcOrg = {});
}  // namespace ftype
