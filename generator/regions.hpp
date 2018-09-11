#pragma once

#include "generator/feature_builder.hpp"
#include "generator/regions/region_info_collector.hpp"

#include "geometry/rect2d.hpp"



#include "base/geo_object_id.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "3party/boost/boost/geometry.hpp"

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
bool GenerateRegions(feature::GenerateInfo const & genInfo); 
}  // namespace generator
