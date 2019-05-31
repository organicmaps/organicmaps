#pragma once

#include "generator/feature_builder.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"

#include <cstdint>

#include <boost/noncopyable.hpp>

namespace generator
{
uint32_t GetPlaceType(FeatureParams const & params);
uint32_t GetPlaceType(feature::FeatureBuilder const & feature);
} // namespace generator
