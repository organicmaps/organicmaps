#include "testing/testing.hpp"

#include "types_helper.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/geometry_holder.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/geo_object_id.hpp"

#include <limits>

namespace feature_builder_test
{
using namespace feature;
using namespace generator::tests_support;
using namespace tests;

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_ManyTypes)
{
  FeatureBuilder fb1;
  FeatureBuilderParams params;

  base::StringIL arr[] = {
      {"building"},
      {"place", "country"},
      {"place", "state"},
      /// @todo Can't realize is it deprecated or we forgot to add clear styles for it.
      //{ "place", "county" },
      {"place", "region"},
      {"place", "city"},
      {"place", "town"},
  };
  AddTypes(params, arr);

  params.FinishAddingTypes();
  params.SetHouseNumberAndHouseName("75", "Best House");
  params.AddName("default", "Name");

  fb1.SetParams(params);
  fb1.SetCenter(m2::PointD(0, 0));

  TEST(fb1.RemoveInvalidTypes(), ());
  TEST(fb1.IsValid(), (fb1));

  FeatureBuilder::Buffer buffer;
  TEST(fb1.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  fb1.SerializeForIntermediate(buffer);

  FeatureBuilder fb2;
  fb2.DeserializeFromIntermediate(buffer);

  TEST(fb2.IsValid(), (fb2));
  TEST_EQUAL(fb1, fb2, ());
  TEST_EQUAL(fb2.GetTypesCount(), 6, ());
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_LineTypes)
{
  FeatureBuilder fb1;
  FeatureBuilderParams params;

  base::StringIL arr[] = {
      {"railway", "rail"},        {"highway", "motorway"},    {"hwtag", "oneway"},
      {"psurface", "paved_good"}, {"junction", "roundabout"},
  };

  AddTypes(params, arr);
  params.FinishAddingTypes();
  fb1.SetParams(params);

  fb1.AssignPoints({{0, 0}, {1, 1}});
  fb1.SetLinear();

  TEST(fb1.RemoveInvalidTypes(), ());
  TEST(fb1.IsValid(), (fb1));

  FeatureBuilder::Buffer buffer;
  TEST(fb1.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  fb1.SerializeForIntermediate(buffer);

  FeatureBuilder fb2;
  fb2.DeserializeFromIntermediate(buffer);

  TEST(fb2.IsValid(), (fb2));
  TEST_EQUAL(fb1, fb2, ());

  TEST_EQUAL(fb2.GetTypesCount(), 5, ());
  ftypes::IsRoundAboutChecker::Instance()(fb2.GetTypes());
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_Waterfall)
{
  FeatureBuilder fb1;
  FeatureBuilderParams params;

  base::StringIL arr[] = {{"waterway", "waterfall"}};
  AddTypes(params, arr);
  TEST(params.FinishAddingTypes(), ());

  fb1.SetParams(params);
  fb1.SetCenter(m2::PointD(1, 1));

  TEST(fb1.RemoveInvalidTypes(), ());
  TEST(fb1.IsValid(), (fb1));

  FeatureBuilder::Buffer buffer;
  TEST(fb1.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  fb1.SerializeForIntermediate(buffer);

  FeatureBuilder fb2;
  fb2.DeserializeFromIntermediate(buffer);

  TEST(fb2.IsValid(), (fb2));
  TEST_EQUAL(fb1, fb2, ());
  TEST_EQUAL(fb2.GetTypesCount(), 1, ());
}

UNIT_CLASS_TEST(TestWithClassificator, FBbuilder_GetMostGeneralOsmId)
{
  FeatureBuilder fb;

  fb.AddOsmId(base::MakeOsmNode(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), base::MakeOsmNode(1), ());

  fb.AddOsmId(base::MakeOsmNode(2));
  fb.AddOsmId(base::MakeOsmWay(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), base::MakeOsmWay(1), ());

  fb.AddOsmId(base::MakeOsmNode(3));
  fb.AddOsmId(base::MakeOsmWay(2));
  fb.AddOsmId(base::MakeOsmRelation(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), base::MakeOsmRelation(1), ());
}

UNIT_CLASS_TEST(TestWithClassificator, FVisibility_RemoveUselessTypes)
{
  Classificator const & c = classif();

  {
    std::vector<uint32_t> types;
    types.push_back(c.GetTypeByPath({"building"}));
    types.push_back(c.GetTypeByPath({"amenity", "theatre"}));

    TEST(RemoveUselessTypes(types, GeomType::Area), ());
    TEST_EQUAL(types.size(), 2, ());
  }

  {
    std::vector<uint32_t> types;
    types.push_back(c.GetTypeByPath({"highway", "primary"}));
    types.push_back(c.GetTypeByPath({"building"}));

    TEST(RemoveUselessTypes(types, GeomType::Area, true /* emptyName */), ());
    TEST_EQUAL(types.size(), 1, ());
    TEST_EQUAL(types[0], c.GetTypeByPath({"building"}), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_RemoveUselessNames)
{
  FeatureBuilderParams params;

  base::StringIL arr[] = {{"boundary", "administrative", "2"}, {"barrier", "fence"}};
  AddTypes(params, arr);
  params.FinishAddingTypes();

  params.AddName("default", "Name");
  params.AddName("ru", "Имя");

  FeatureBuilder fb1;
  fb1.SetParams(params);

  fb1.AssignPoints({{0, 0}, {1, 1}});
  fb1.SetLinear();

  TEST(!fb1.GetName(0).empty(), ());
  TEST(!fb1.GetName(8).empty(), ());

  fb1.RemoveUselessNames();

  TEST(fb1.GetName(0).empty(), ());
  TEST(fb1.GetName(8).empty(), ());

  TEST(fb1.IsValid(), (fb1));
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_HN)
{
  FeatureBuilderParams params;

  params.MakeZero();
  TEST(params.AddHouseNumber("123"), ());
  TEST_EQUAL(params.house.Get(), "123", ());

  params.MakeZero();
  TEST(params.AddHouseNumber("00123"), ());
  TEST_EQUAL(params.house.Get(), "00123", ());

  params.MakeZero();
  TEST(params.AddHouseNumber("0"), ());
  TEST_EQUAL(params.house.Get(), "0", ());
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_SerializeLocalityObjectForBuildingPoint)
{
  FeatureBuilder fb;
  FeatureBuilderParams params;

  base::StringIL arr[] = {
      {"building"},
  };
  AddTypes(params, arr);

  params.FinishAddingTypes();
  params.SetHouseNumberAndHouseName("75", "Best House");
  params.AddName("default", "Name");

  fb.AddOsmId(base::MakeOsmNode(1));
  fb.SetParams(params);
  fb.SetCenter(m2::PointD(10.1, 15.8));

  TEST(fb.RemoveInvalidTypes(), ());
  TEST(fb.IsValid(), (fb));

  feature::DataHeader header;
  header.SetGeometryCodingParams(serial::GeometryCodingParams());
  header.SetScales({scales::GetUpperScale()});
  feature::GeometryHolder holder(fb, header);

  auto & buffer = holder.GetBuffer();
  TEST(fb.PreSerializeAndRemoveUselessNamesForMwm(buffer), ());
}

UNIT_TEST(LooksLikeHouseNumber)
{
  TEST(FeatureParams::LooksLikeHouseNumber("1 bis"), ());
  TEST(FeatureParams::LooksLikeHouseNumber("18-20"), ());

  // Brno (Czech) has a lot of fancy samples.
  TEST(FeatureParams::LooksLikeHouseNumber("ev.8"), ());
  TEST(FeatureParams::LooksLikeHouseNumber("D"), ());
  TEST(FeatureParams::LooksLikeHouseNumber("A5"), ());

  TEST(!FeatureParams::LooksLikeHouseNumber("Building 2"), ());
  TEST(!FeatureParams::LooksLikeHouseNumber("Unit 3"), ());
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_HouseName)
{
  FeatureBuilder fb;
  FeatureBuilderParams params;

  base::StringIL arr[] = {{"building"}};
  AddTypes(params, arr);
  params.FinishAddingTypes();

  params.SetHouseNumberAndHouseName("", "St. Nicholas Lodge");

  fb.SetParams(params);
  fb.AssignArea({{0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}}, {});
  fb.SetArea();

  TEST(fb.RemoveInvalidTypes(), ());
  TEST(fb.IsValid(), ());

  TEST(fb.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  TEST(fb.IsValid(), ());
  TEST_EQUAL(fb.GetName(StringUtf8Multilang::kDefaultCode), "St. Nicholas Lodge", ());
  TEST(fb.GetParams().house.IsEmpty(), ());
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_SerializeAccuratelyForIntermediate)
{
  FeatureBuilder fb1;
  FeatureBuilderParams params;

  base::StringIL arr[] = {
      {"railway", "rail"},        {"highway", "motorway"},  {"hwtag", "oneway"},
      {"psurface", "paved_good"}, {"junction", "circular"},
  };

  AddTypes(params, arr);
  params.FinishAddingTypes();
  fb1.SetParams(params);

  auto const diff = 0.33333333334567;
  std::vector<m2::PointD> points;
  for (size_t i = 0; i < 100; ++i)
    points.push_back({i + diff, i + 1 + diff});

  fb1.AssignPoints(std::move(points));
  fb1.SetLinear();

  TEST(fb1.RemoveInvalidTypes(), ());
  TEST(fb1.IsValid(), (fb1));

  FeatureBuilder::Buffer buffer;
  TEST(fb1.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  fb1.SerializeAccuratelyForIntermediate(buffer);

  FeatureBuilder fb2;
  fb2.DeserializeAccuratelyFromIntermediate(buffer);

  TEST(fb2.IsValid(), (fb2));
  TEST(fb1.IsExactEq(fb2), ());

  TEST_EQUAL(fb2.GetTypesCount(), 5, ());
  ftypes::IsRoundAboutChecker::Instance()(fb2.GetTypes());
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_RemoveUselessAltName)
{
  auto const kDefault = StringUtf8Multilang::kDefaultCode;
  auto const kAltName = StringUtf8Multilang::GetLangIndex("alt_name");

  {
    FeatureBuilderParams params;

    base::StringIL arr[] = {{"shop"}};
    AddTypes(params, arr);
    params.FinishAddingTypes();

    // We should remove alt_name which is almost equal to name.
    params.AddName("default", "Перекрёсток");
    params.AddName("alt_name", "Перекресток");

    FeatureBuilder fb;
    fb.SetParams(params);

    fb.SetCenter(m2::PointD(0.0, 0.0));

    TEST(!fb.GetName(kDefault).empty(), ());
    TEST(!fb.GetName(kAltName).empty(), ());

    fb.RemoveUselessNames();

    TEST(!fb.GetName(kDefault).empty(), ());
    TEST(fb.GetName(kAltName).empty(), ());

    TEST(fb.IsValid(), (fb));
  }
  {
    FeatureBuilderParams params;

    base::StringIL arr[] = {{"shop"}};
    AddTypes(params, arr);
    params.FinishAddingTypes();

    // We should not remove alt_name which differs from name.
    params.AddName("default", "Государственный Универсальный Магазин");
    params.AddName("alt_name", "ГУМ");

    FeatureBuilder fb;
    fb.SetParams(params);

    fb.SetCenter(m2::PointD(0.0, 0.0));

    TEST(!fb.GetName(kDefault).empty(), ());
    TEST(!fb.GetName(StringUtf8Multilang::GetLangIndex("alt_name")).empty(), ());

    fb.RemoveUselessNames();

    TEST(!fb.GetName(kDefault).empty(), ());
    TEST(!fb.GetName(kAltName).empty(), ());

    TEST(fb.IsValid(), (fb));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_RemoveInconsistentTypes)
{
  FeatureBuilderParams params;

  base::StringIL arr[] = {
      {"highway", "cycleway"}, {"hwtag", "onedir_bicycle"}, {"hwtag", "nobicycle"}, {"hwtag", "yesbicycle"}};
  AddTypes(params, arr);
  TEST_EQUAL(params.m_types.size(), 4, ());

  TEST(params.RemoveInconsistentTypes(), ());
  TEST_EQUAL(params.m_types.size(), 3, ());
  TEST(!params.IsTypeExist(classif().GetTypeByPath({"hwtag", "nobicycle"})), ());
}

}  // namespace feature_builder_test
