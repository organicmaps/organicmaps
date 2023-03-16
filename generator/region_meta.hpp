#pragma once

#include "generate_info.hpp"

#include "indexer/feature_meta.hpp"

#include <string>

namespace feature
{
bool ReadRegionData(std::string const & countryName, RegionData & data);
}  // namespace feature
