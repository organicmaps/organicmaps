#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"

namespace generator
{
uint32_t GetPlaceType(FeatureParams const & params)
{
  auto static const placeType = classif().GetTypeByPath({"place"});
  return params.FindType(placeType, 1 /* level */);
}

uint32_t GetPlaceType(FeatureBuilder1 const & feature)
{
  return GetPlaceType(feature.GetParams());
}
}  // namespace generator
