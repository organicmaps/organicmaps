#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"

namespace generator
{

uint32_t GetPlaceType(FeatureParams const & params)
{
  auto static const placeType = classif().GetTypeByPath({"place"});
  return params.FindType(placeType, 1 /* level */);
}

uint32_t GetPlaceType(feature::FeatureBuilder const & fb)
{
  return GetPlaceType(fb.GetParams());
}

bool IsRealCapital(feature::FeatureBuilder const & fb)
{
  auto static const capitalType = classif().GetTypeByPath({"place", "city", "capital", "2"});
  return fb.GetParams().IsTypeExist(capitalType);
}

}  // namespace generator
