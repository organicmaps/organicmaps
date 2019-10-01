#include "generator/filter_complex.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/ftypes_matcher.hpp"

namespace generator
{
std::shared_ptr<FilterInterface> FilterComplex::Clone() const
{
  return std::make_shared<FilterComplex>();
}

bool FilterComplex::IsAccepted(feature::FeatureBuilder const & fb)
{
  if (!fb.IsArea() && !fb.IsPoint())
    return false;

  auto const & eatChecker = ftypes::IsEatChecker::Instance();
  auto const & attractChecker = ftypes::AttractionsChecker::Instance();
  auto const & airportChecker = ftypes::IsAirportChecker::Instance();
  auto const & ts = fb.GetTypes();
  return eatChecker(ts) || attractChecker(ts) || airportChecker(ts);
}
}  // namespace generator
