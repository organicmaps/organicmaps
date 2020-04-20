#include "generator/filter_complex.hpp"

#include "generator/feature_builder.hpp"
#include "generator/hierarchy.hpp"

#include "indexer/ftypes_matcher.hpp"

namespace generator
{
std::shared_ptr<FilterInterface> FilterComplex::Clone() const
{
  return std::make_shared<FilterComplex>();
}

bool FilterComplex::IsAccepted(feature::FeatureBuilder const & fb) const
{
  if (!fb.IsArea() && !fb.IsPoint())
    return false;

  return hierarchy::GetMainType(fb.GetTypes()) != ftype::GetEmptyValue();
}
}  // namespace generator
