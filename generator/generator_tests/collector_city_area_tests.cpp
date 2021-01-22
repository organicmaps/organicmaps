#include "testing/testing.hpp"

#include "generator/collector_city_area.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generator_tests/common.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "geometry/point2d.hpp"

#include "base/geo_object_id.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <memory>
#include <vector>

using namespace generator_tests;

namespace
{
feature::FeatureBuilder MakeFbForTest(OsmElement element)
{
  feature::FeatureBuilder result;
  ftype::GetNameAndType(&element, result.GetParams());
  std::vector<m2::PointD> polygon = {{0, 0}, {0, 2}, {2, 2}, {2, 0}, {0, 0}};
  result.SetOsmId(base::MakeOsmRelation(element.m_id));
  result.AddPolygon(polygon);
  result.SetArea();
  return result;
}

bool HasRelationWithId(std::vector<feature::FeatureBuilder> const & fbs, uint64_t id) {
  return base::FindIf(fbs, [&](auto const & fb) {
    return fb.GetMostGenericOsmId() == base::MakeOsmRelation(id);
  }) != std::end(fbs);
};

auto const o1 = MakeOsmElement(1 /* id */, {{"place", "city"}} /* tags */, OsmElement::EntityType::Relation);
auto const o2 = MakeOsmElement(2 /* id */, {{"place", "town"}} /* tags */, OsmElement::EntityType::Relation);
auto const o3 = MakeOsmElement(3 /* id */, {{"place", "village"}} /* tags */, OsmElement::EntityType::Relation);
auto const o4 = MakeOsmElement(4 /* id */, {{"place", "country"}} /* tags */, OsmElement::EntityType::Relation);
}  // namespace

UNIT_TEST(CollectorCityArea_Merge)
{
  classificator::Load();
  auto const filename = generator_tests::GetFileName();
  SCOPE_GUARD(_, std::bind(Platform::RemoveFileIfExists, std::cref(filename)));

  auto c1 = std::make_shared<generator::CityAreaCollector>(filename);
  auto c2 = c1->Clone();
  c1->CollectFeature(MakeFbForTest(o1), o1);
  c2->CollectFeature(MakeFbForTest(o2), o2);
  c1->CollectFeature(MakeFbForTest(o3), o3);
  c2->CollectFeature(MakeFbForTest(o4), o4);
  c1->Finish();
  c2->Finish();
  c1->Merge(*c2);
  c1->Finalize();

  auto const fbs = feature::ReadAllDatRawFormat<feature::serialization_policy::MaxAccuracy>(filename);
  TEST_EQUAL(fbs.size(), 3, ());
  TEST(HasRelationWithId(fbs, 1 /* id */), ());
  TEST(HasRelationWithId(fbs, 2 /* id */), ());
  TEST(HasRelationWithId(fbs, 3 /* id */), ());
}
