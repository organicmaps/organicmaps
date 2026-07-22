#include "testing/testing.hpp"

#include "drape_frontend/apply_feature_functors.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

namespace bicycle_line_types_tests
{
feature::TypesHolder MakeTypes(std::initializer_list<base::StringIL> const & paths)
{
  feature::TypesHolder types(feature::GeomType::Line);
  for (auto const & path : paths)
    types.Add(classif().GetTypeByPath(path));
  return types;
}

UNIT_TEST(BicycleLineTypes_AccessOnly)
{
  classificator::Load();

  TEST(df::GetBicycleLineKind(MakeTypes({{"highway", "secondary"}, {"hwtag", "yesbicycle"}})) ==
           df::BicycleLineKind::None,
       ());
  TEST(df::GetBicycleLineKind(MakeTypes({{"highway", "residential"}, {"hwtag", "yesbicycle"}})) ==
           df::BicycleLineKind::SharedLane,
       ());
  TEST(df::GetBicycleLineKind(MakeTypes({{"highway", "path"}, {"hwtag", "yesbicycle"}})) == df::BicycleLineKind::None,
       ());
}

UNIT_TEST(BicycleLineTypes_Infrastructure)
{
  classificator::Load();

  TEST(df::GetBicycleLineKind(MakeTypes({{"highway", "secondary"}, {"cyclewaytag", "lane"}})) ==
           df::BicycleLineKind::Lane,
       ());
  TEST(df::GetBicycleLineKind(MakeTypes({{"highway", "secondary"}, {"cyclewaytag", "track"}})) ==
           df::BicycleLineKind::Track,
       ());
  TEST(df::GetBicycleLineKind(MakeTypes({{"highway", "path", "bicycle"}})) == df::BicycleLineKind::Cycleway, ());
}
}  // namespace bicycle_line_types_tests
