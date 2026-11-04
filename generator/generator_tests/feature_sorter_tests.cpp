#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_helpers.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/features_vector.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace feature_sorter_tests
{
using namespace generator::tests_support;
using Header = feature::DataHeader;
using FeatureGroups = std::array<std::vector<TestFeature const *>, static_cast<size_t>(Header::FeatureGroup::Count)>;

namespace
{
uint64_t GetSortKey(TestFeature const & testFeature)
{
  feature::FeatureBuilder fb;
  testFeature.Serialize(fb);
  m2::PointD middle = m2::PointD::Zero();
  size_t count = 0;
  fb.ForEachPoint([&](m2::PointD const & point)
  {
    middle += point;
    ++count;
  });
  auto const scale = feature::GetMinDrawableScale(fb.GetTypesHolder(), fb.GetLimitRect());
  TEST_NOT_EQUAL(scale, -1, (testFeature));
  // Order each group by min drawable scale, then by the middle point Morton code.
  return (static_cast<uint64_t>(scale) << 59) |
         (PointToInt64Obsolete(middle / count, serial::GeometryCodingParams().GetCoordBits()) >> 5);
}

void CheckRangesAndFeatures(FeaturesVectorTest const & features, FeatureGroups groups)
{
  auto const & header = features.GetHeader();
  std::vector<TestFeature const *> expected;
  for (size_t i = 0; i < groups.size(); ++i)
  {
    auto & group = groups[i];
    std::sort(group.begin(), group.end(),
              [](auto const * lhs, auto const * rhs) { return GetSortKey(*lhs) < GetSortKey(*rhs); });
    auto const range = header.GetFeatureRange(static_cast<Header::FeatureGroup>(i));
    TEST_EQUAL(range.first, expected.size(), (i));
    expected.insert(expected.end(), group.begin(), group.end());
    TEST_EQUAL(range.second, expected.size(), (i));
  }
  TEST_EQUAL(header.GetFeatureCount(), expected.size(), ());
  TEST_EQUAL(features.GetVector().GetNumFeatures(), expected.size(), ());

  uint32_t count = 0;
  features.GetVector().ForEach([&](FeatureType & ft, uint32_t index)
  {
    TEST_EQUAL(index, count, ());
    ++count;
    TEST_LESS(index, expected.size(), ());
    auto const & source = *expected[index];
    TEST(source.Matches(ft), (index, source));

    feature::FeatureBuilder fb;
    source.Serialize(fb);
    auto const bits = header.GetDefGeometryCodingParams().GetCoordBits();
    auto const quantize = [bits](m2::PointD const & point)
    { return PointUToPointD(PointDToPointU(point, bits), bits); };
    if (fb.IsPoint())
    {
      TEST_EQUAL(ft.GetCenter(), quantize(source.GetCenter()), (index));
    }
    else if (fb.IsLine())
    {
      auto const & points = ft.GetPoints(FeatureType::BEST_GEOMETRY);
      auto const & original = fb.GetOuterGeometry();
      TEST_EQUAL(points.size(), original.size(), (index));
      for (size_t j = 0; j < points.size(); ++j)
        TEST_EQUAL(points[j], quantize(original[j]), (index, j));
    }
  });
  TEST_EQUAL(count, expected.size(), ());
}
}  // namespace

UNIT_CLASS_TEST(TestWithCustomMwms, FeatureSorter_RangesAndOrder)
{
  TestStreet street({{12, 13}, {14, 15}}, "Street", "en");
  TestSquare square({20, 21, 22, 23}, "Square", "en");
  TestBuilding address(m2::PointD{31, 32}, "", "10", "en");
  TestBuilding addressArea(m2::RectD{34, 35, 36, 37}, "", "11", "", "en");
  TestBuilding namedHouse(m2::PointD{41, 42}, "Named house", "12", "en");
  TestPOI cafe({43, 44}, "", "en");
  cafe.SetTypes({{"amenity", "cafe"}});
  TestPlace locality(m2::PointD{51, 52}, "Locality", "en", classif().GetTypeByPath({"place", "locality"}));
  TestSuburb suburb({53, 54}, "Suburb", "en");
  TestBuilding building(m2::RectD{61, 62, 63, 64}, "", "", "", "en");
  TestBuilding forest(m2::RectD{65, 66, 67, 68}, "", "", "", "en");
  forest.SetType(classif().GetTypeByPath({"landuse", "forest"}));
  TestBuilding degenerate(m2::RectD{71, 72, 73, 72}, "", "13", "", "en");

  std::array<TestFeature const *, 11> const input = {&building,   &locality, &cafe,   &address,    &square,     &forest,
                                                     &namedHouse, &street,   &suburb, &degenerate, &addressArea};
  m2::PointD sum = m2::PointD::Zero();
  size_t pointsCount = 0;
  auto const id = BuildCountry("feature-sorter-ranges", [&](TestMwmBuilder & builder)
  {
    for (auto const * testFeature : input)
    {
      feature::FeatureBuilder fb;
      testFeature->Serialize(fb);
      fb.ForEachPoint([&](m2::PointD const & point)
      {
        sum += point;
        ++pointsCount;
      });
      if (testFeature == &degenerate)
      {
        feature::CalculateMidPoints midPoints;
        midPoints(fb, 0, Header::FeatureGroup::Pois);
        TEST_EQUAL(midPoints.GetVector(Header::FeatureGroup::Pois).size(), 1, ());
      }
      TEST(builder.Add(fb), (*testFeature));
    }
  });

  FeaturesVectorTest features(id.GetInfo()->GetLocalFile().GetPath(MapFileType::Map));
  FeatureGroups const expected = {
      {{&street, &square}, {&address, &addressArea, &namedHouse, &cafe}, {&locality, &suburb}, {&building, &forest}}};
  CheckRangesAndFeatures(features, expected);

  // The coding origin includes all raw points, including geometry discarded during serialization.
  serial::GeometryCodingParams const coding(kFeatureSorterPointCoordBits, sum / pointsCount);
  TEST_EQUAL(features.GetHeader().GetDefGeometryCodingParams().GetBasePointUint64(), coding.GetBasePointUint64(), ());
}

UNIT_CLASS_TEST(TestWithCustomMwms, FeatureSorter_EmptyRanges)
{
  TestPOI cafe({17, 19}, "", "en");
  cafe.SetTypes({{"amenity", "cafe"}});
  auto const id = BuildCountry("feature-sorter-empty-ranges", [&](TestMwmBuilder & builder) { builder.Add(cafe); });
  FeaturesVectorTest features(id.GetInfo()->GetLocalFile().GetPath(MapFileType::Map));
  FeatureGroups const expected = {{{}, {&cafe}, {}, {}}};
  CheckRangesAndFeatures(features, expected);
}
}  // namespace feature_sorter_tests
