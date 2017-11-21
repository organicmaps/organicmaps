#include "testing/testing.hpp"

#include "types_helper.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm2type.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature_visibility.hpp"

using namespace tests;


UNIT_TEST(FBuilder_ManyTypes)
{
  classificator::Load();

  FeatureBuilder1 fb1;
  FeatureParams params;

  char const * arr1[][1] = {
    { "building" },
  };
  AddTypes(params, arr1);

  char const * arr2[][2] = {
    { "place", "country" },
    { "place", "state" },
    /// @todo Can't realize is it deprecated or we forgot to add clear styles for it.
    //{ "place", "county" },
    { "place", "region" },
    { "place", "city" },
    { "place", "town" },
  };
  AddTypes(params, arr2);

  params.FinishAddingTypes();
  params.AddHouseNumber("75");
  params.AddHouseName("Best House");
  params.AddName("default", "Name");

  fb1.SetParams(params);
  fb1.SetCenter(m2::PointD(0, 0));

  TEST(fb1.RemoveInvalidTypes(), ());
  TEST(fb1.CheckValid(), ());

  FeatureBuilder1::TBuffer buffer;
  TEST(fb1.PreSerialize(), ());
  fb1.Serialize(buffer);

  FeatureBuilder1 fb2;
  fb2.Deserialize(buffer);

  TEST(fb2.CheckValid(), ());
  TEST_EQUAL(fb1, fb2, ());
  TEST_EQUAL(fb2.GetTypesCount(), 6, ());
}

UNIT_TEST(FBuilder_LineTypes)
{
  FeatureBuilder1 fb1;
  FeatureParams params;

  char const * arr2[][2] = {
    { "railway", "rail" },
    { "highway", "motorway" },
    { "hwtag", "oneway" },
    { "psurface", "paved_good" },
    { "junction", "roundabout" },
  };

  AddTypes(params, arr2);
  params.FinishAddingTypes();
  fb1.SetParams(params);

  fb1.AddPoint(m2::PointD(0, 0));
  fb1.AddPoint(m2::PointD(1, 1));
  fb1.SetLinear();

  TEST(fb1.RemoveInvalidTypes(), ());
  TEST(fb1.CheckValid(), ());

  FeatureBuilder1::TBuffer buffer;
  TEST(fb1.PreSerialize(), ());
  fb1.Serialize(buffer);

  FeatureBuilder1 fb2;
  fb2.Deserialize(buffer);

  TEST(fb2.CheckValid(), ());
  TEST_EQUAL(fb1, fb2, ());
  TEST_EQUAL(fb2.GetTypesCount(), 5, ());
}

UNIT_TEST(FBuilder_Waterfall)
{
  classificator::Load();

  FeatureBuilder1 fb1;
  FeatureParams params;

  char const * arr[][2] = {{"waterway", "waterfall"}};
  AddTypes(params, arr);
  TEST(params.FinishAddingTypes(), ());

  fb1.SetParams(params);
  fb1.SetCenter(m2::PointD(1, 1));

  TEST(fb1.RemoveInvalidTypes(), ());
  TEST(fb1.CheckValid(), ());

  FeatureBuilder1::TBuffer buffer;
  TEST(fb1.PreSerialize(), ());
  fb1.Serialize(buffer);

  FeatureBuilder1 fb2;
  fb2.Deserialize(buffer);

  TEST(fb2.CheckValid(), ());
  TEST_EQUAL(fb1, fb2, ());
  TEST_EQUAL(fb2.GetTypesCount(), 1, ());
}

UNIT_TEST(FBbuilder_GetMostGeneralOsmId)
{
  FeatureBuilder1 fb;

  fb.AddOsmId(osm::Id::Node(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), osm::Id::Node(1), ());

  fb.AddOsmId(osm::Id::Node(2));
  fb.AddOsmId(osm::Id::Way(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), osm::Id::Way(1), ());

  fb.AddOsmId(osm::Id::Node(3));
  fb.AddOsmId(osm::Id::Way(2));
  fb.AddOsmId(osm::Id::Relation(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), osm::Id::Relation(1), ());
}

UNIT_TEST(FVisibility_RemoveNoDrawableTypes)
{
  classificator::Load();
  Classificator const & c = classif();

  {
    vector<uint32_t> types;
    types.push_back(c.GetTypeByPath({ "building" }));
    types.push_back(c.GetTypeByPath({ "amenity", "theatre" }));

    TEST(feature::RemoveNoDrawableTypes(types, feature::GEOM_AREA), ());
    TEST_EQUAL(types.size(), 2, ());
  }

  {
    vector<uint32_t> types;
    types.push_back(c.GetTypeByPath({ "highway", "primary" }));
    types.push_back(c.GetTypeByPath({ "building" }));

    TEST(feature::RemoveNoDrawableTypes(types, feature::GEOM_AREA, true), ());
    TEST_EQUAL(types.size(), 1, ());
    TEST_EQUAL(types[0], c.GetTypeByPath({ "building" }), ());
  }
}

UNIT_TEST(FBuilder_RemoveUselessNames)
{
  classificator::Load();

  FeatureParams params;

  char const * arr3[][3] = { { "boundary", "administrative", "2" } };
  AddTypes(params, arr3);
  char const * arr2[][2] = { { "barrier", "fence" } };
  AddTypes(params, arr2);
  params.FinishAddingTypes();

  params.AddName("default", "Name");
  params.AddName("ru", "Имя");

  FeatureBuilder1 fb1;
  fb1.SetParams(params);

  fb1.AddPoint(m2::PointD(0, 0));
  fb1.AddPoint(m2::PointD(1, 1));
  fb1.SetLinear();

  TEST(!fb1.GetName(0).empty(), ());
  TEST(!fb1.GetName(8).empty(), ());

  fb1.RemoveUselessNames();

  TEST(fb1.GetName(0).empty(), ());
  TEST(fb1.GetName(8).empty(), ());

  TEST(fb1.CheckValid(), ());
}

UNIT_TEST(FeatureParams_Parsing)
{
  classificator::Load();

  {
    FeatureParams params;
    params.AddStreet("Embarcadero\nstreet");
    TEST_EQUAL(params.GetStreet(), "Embarcadero street", ());
  }

  {
    FeatureParams params;
    params.AddAddress("165 \t\t Dolliver Street");
    TEST_EQUAL(params.GetStreet(), "Dolliver Street", ());
  }

  {
    FeatureParams params;

    params.MakeZero();
    TEST(params.AddHouseNumber("123"), ());
    TEST_EQUAL(params.house.Get(), "123", ());

    params.MakeZero();
    TEST(params.AddHouseNumber("0000123"), ());
    TEST_EQUAL(params.house.Get(), "123", ());

    params.MakeZero();
    TEST(params.AddHouseNumber("000000"), ());
    TEST_EQUAL(params.house.Get(), "0", ());
  }
}
