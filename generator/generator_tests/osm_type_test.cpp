#include "testing/testing.hpp"

#include "generator/generator_tests/types_helper.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm2type.hpp"
#include "generator/tag_admixer.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <string>
#include <vector>

namespace osm_type_test
{
using namespace generator::tests_support;
using namespace tests;

using Tags = std::vector<OsmElement::Tag>;

void TestSurfaceTypes(std::string const & surface, std::string const & smoothness,
                      std::string const & grade, char const * value)
{
  OsmElement e;
  e.AddTag("highway", "unclassified");
  e.AddTag("surface", surface);
  e.AddTag("smoothness", smoothness);
  e.AddTag("surface:grade", grade);

  FeatureBuilderParams params;
  ftype::GetNameAndType(&e, params);

  TEST_EQUAL(params.m_types.size(), 2, (params));
  TEST(params.IsTypeExist(GetType({"highway", "unclassified"})), ());
  std::string psurface;
  for (auto type : params.m_types)
  {
    std::string const rtype = classif().GetReadableObjectName(type);
    if (rtype.substr(0, 9) == "psurface-")
      psurface = rtype.substr(9);
  }
  TEST(params.IsTypeExist(GetType({"psurface", value})),
        ("Surface:", surface, "Smoothness:", smoothness, "Grade:", grade, "Expected:", value,
        "Got:", psurface));
}

FeatureBuilderParams GetFeatureBuilderParams(
    Tags const & tags,
    OsmElement::EntityType type = OsmElement::EntityType::Unknown)
{
  OsmElement e;
  e.m_type = type;
  FillXmlElement(tags, &e);
  FeatureBuilderParams params;

  static TagReplacer tagReplacer(base::JoinPath(GetPlatform().ResourcesDir(), REPLACED_TAGS_FILE));
  tagReplacer.Process(e);

  ftype::GetNameAndType(&e, params);
  return params;
}


UNIT_CLASS_TEST(TestWithClassificator, OsmType_SkipDummy)
{
  Tags const tags = {
    { "abutters", "residential" },
    { "highway", "primary" },
    { "osmarender:renderRef", "no" },
    { "ref", "E51" }
  };

  auto const params = GetFeatureBuilderParams(tags);

  TEST_EQUAL(params.m_types.size(), 1, (params));
  TEST_EQUAL(params.m_types[0], GetType({"highway", "primary"}), ());
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Oneway)
{
  {
    Tags const tags = {
      { "highway", "primary" },
      { "motorroad", "yes" },
      { "name", "Каширское шоссе" },
      { "oneway", "yes" }
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"highway", "primary"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "oneway"})), ());
  }

  {
    Tags const tags = {
      { "highway", "primary" },
      { "name", "Каширское шоссе" },
      { "oneway", "-1" },
      { "motorroad", "yes" }
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"highway", "primary"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "oneway"})), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Location)
{
  {
    Tags const tags = {
      { "power", "line" },
      { "location", "underground" },
      { "man_made", "pipeline" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 0, (params));
  }

  {
    Tags const tags = {
      { "power", "line" },
      { "man_made", "pipeline" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"power", "line"})), ());
    // We don't have drawing rules now for pipeline.
    //TEST(params.IsTypeExist(GetType({"man_made", "pipeline"})), ());
  }

  {
    Tags const tags = {
      { "power", "line" },
      { "location", "overground" },
      { "man_made", "pipeline" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    /// @todo Mapcss understands only [!location] syntax now. Make it possible to set [location!=underground]
    TEST_EQUAL(params.m_types.size(), 0, (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Combined)
{
  Tags const tags = {
    { "addr:housenumber", "84" },
    { "addr:postcode", "220100" },
    { "addr:street", "ул. Максима Богдановича" },
    { "amenity", "school" },
    { "building", "yes" },
    { "name", "Гимназия 15" }
  };

  auto const params = GetFeatureBuilderParams(tags);

  TEST_EQUAL(params.m_types.size(), 2, (params));
  TEST(params.IsTypeExist(GetType({"amenity", "school"})), ());
  TEST(params.IsTypeExist(GetType({"building"})), ());

  std::string_view s;
  params.name.GetString(0, s);
  TEST_EQUAL(s, "Гимназия 15", ());

  TEST_EQUAL(params.house.Get(), "84", ());
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Address)
{
  uint32_t const addrType = GetType({"building", "address"});
  {
    // Single house number tag is transformed into address type.
    Tags const tags = { {"addr:housenumber", "42"} };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(addrType), ());

    TEST_EQUAL(params.house.Get(), "42", ());
  }

  {
    Tags const tags = {
      { "addr:conscriptionnumber", "223" },
      { "addr:housenumber", "223/5" },
      { "addr:postcode", "11000" },
      { "addr:street", "Řetězová" },
      { "addr:streetnumber", "5" },
      { "source:addr", "uir_adr" },
      { "uir_adr:ADRESA_KOD", "21717036" }
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(addrType), ());

    TEST_EQUAL(params.house.Get(), "223/5", ());
    TEST_EQUAL(params.GetStreet(), "Řetězová", ());
    TEST_EQUAL(params.GetPostcode(), "11000", ());
  }

  {
    Tags const tags = {
      {"addr:city", "Zürich"},
      {"addr:housenumber", "41"},
      {"addr:postcode", "8050"},
      {"addr:street", "Leutschenbachstrasse"},
      {"entrance", "main"},
      {"survey:date", "2020-12-17"},
      {"wheelchair", "no"},
      {"internet_access", "wlan"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"entrance", "main"})), ());
    TEST(params.IsTypeExist(GetType({"wheelchair", "no"})), ());
    TEST(params.IsTypeExist(GetType({"internet_access", "wlan"})), ());

    TEST_EQUAL(params.house.Get(), "41", ());
    TEST_EQUAL(params.GetStreet(), "Leutschenbachstrasse", ());
    TEST_EQUAL(params.GetPostcode(), "8050", ());
  }

  {
    Tags const tags = {
      {"addr:city", "Šķaune"},
      {"addr:country", "LV"},
      {"addr:district", "Krāslavas novads"},
      {"addr:housename", "Rozemnieki"},
      {"addr:postcode", "LV-5695"},
      {"addr:subdistrict", "Šķaunes pagasts"},
      {"ref:LV:addr", "104934702"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(addrType), ());

    TEST_EQUAL(params.house.Get(), "Rozemnieki", ());
    TEST(params.GetStreet().empty(), ());
    TEST_EQUAL(params.GetPostcode(), "LV-5695", ());
  }

  {
    Tags const tags = {
      {"building", "yes"},
      {"contact:city", "Paris"},
      {"contact:housenumber", "13"},
      {"contact:phone", "+33 1 44 77 60 60"},
      {"contact:postcode", "75001"},
      {"contact:street", "Place Vendôme"},
      {"contact:website", "https://www.justice.gouv.fr/"},
      {"government", "ministry"},
      {"historic", "manor"},
      {"layer", "1"},
      {"name", "Ministère de la Justice"},
      {"office", "government"},
      {"wikidata", "Q3145763"},
    };

    auto const params = GetFeatureBuilderParams(tags);
    TEST_EQUAL(params.m_types.size(), 3, (params));

    TEST_EQUAL(params.house.Get(), "13", ());
    TEST_EQUAL(params.GetStreet(), "Place Vendôme", ());
    TEST_EQUAL(params.GetPostcode(), "75001", ());
  }

  {
    Tags const tags = {
      {"addr:city", "München"},
      {"addr:country", "DE"},
      {"addr:housenumber", "27"},
      {"addr:postcode", "80339"},
      {"addr:street", "Ligsalzstraße"},
      {"clothes", "children"},
      {"disused:shop", "clothes"},
      {"name", "Westendprinz"},
      {"operator", "Meike Hannig"},
    };

    auto const params = GetFeatureBuilderParams(tags);
    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(addrType), ());

    TEST_EQUAL(params.house.Get(), "27", ());
    TEST_EQUAL(params.GetStreet(), "Ligsalzstraße", ());
    TEST_EQUAL(params.GetPostcode(), "80339", ());

    TEST(params.name.IsEmpty(), ());
    TEST(!params.GetMetadata().Has(feature::Metadata::FMD_OPERATOR), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_PlaceState)
{
  Tags const tags = {
    { "alt_name:vi", "California" },
    { "is_in", "USA" },
    { "is_in:continent", "North America" },
    { "is_in:country", "USA" },
    { "is_in:country_code", "us" },
    { "name", "California" },
    { "place", "state" },
    { "population", "37253956" },
    { "ref", "CA" }
  };

  auto const params = GetFeatureBuilderParams(tags);

  TEST_EQUAL(params.m_types.size(), 1, (params));
  TEST(params.IsTypeExist(GetType({"place", "state", "USA"})), ());

  std::string_view s;
  TEST(params.name.GetString(0, s), ());
  TEST_EQUAL(s, "California", ());
  TEST_GREATER(params.rank, 1, ());
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_AlabamaRiver)
{
  Tags const tags1 = {
    { "NHD:FCode", "55800" },
    { "NHD:FType", "558" },
    { "NHD:RESOLUTION", "2" },
    { "NHD:way_id", "139286586;139286577;139286596;139286565;139286574;139286508;139286600;139286591;139286507;139286505;139286611;139286602;139286594;139286604;139286615;139286616;139286608;139286514;139286511;139286564;139286576;139286521;139286554" },
    { "attribution", "NHD" },
    { "boat", "yes" },
    { "deep_draft", "no" },
    { "gnis:feature_id", "00517033" },
    { "name", "Tennessee River" },
    { "ship", "yes" },
    { "source", "NHD_import_v0.4_20100913205417" },
    { "source:deep_draft", "National Transportation Atlas Database 2011" },
    { "waterway", "river" }
  };

  Tags const tags2 = {
    { "destination", "Ohio River" },
    { "name", "Tennessee River" },
    { "type", "waterway" },
    { "waterway", "river" }
  };

  Tags const tags3 = {
    { "name", "Tennessee River" },
    { "network", "inland waterways" },
    { "route", "boat" },
    { "ship", "yes" },
    { "type", "route" }
  };

  OsmElement e;
  FillXmlElement(tags1, &e);
  FillXmlElement(tags2, &e);
  FillXmlElement(tags3, &e);

  FeatureBuilderParams params;
  ftype::GetNameAndType(&e, params);

  TEST_EQUAL(params.m_types.size(), 1, (params));
  TEST(params.IsTypeExist(GetType({"waterway", "river"})), ());
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Synonyms)
{
  // Smoke test.
  {
    Tags const tags = {
      { "building", "yes" },
      { "atm", "yes" },
      { "shop", "yes" },
      { "restaurant", "yes" },
      { "hotel", "yes" },
      { "toilets", "yes" },
      { "drinkable", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 7, (params));

    TEST(params.IsTypeExist(GetType({"building"})), ());
    TEST(params.IsTypeExist(GetType({"amenity", "atm"})), ());
    TEST(params.IsTypeExist(GetType({"shop"})), ());
    TEST(params.IsTypeExist(GetType({"amenity", "restaurant"})), ());
    TEST(params.IsTypeExist(GetType({"tourism", "hotel"})), ());
    TEST(params.IsTypeExist(GetType({"toilets", "yes"})), ());
    TEST(params.IsTypeExist(GetType({"drinking_water", "yes"})), ());
  }

  // Duplicating test.
  {
    Tags const tags = {
      { "amenity", "atm" },
      { "atm", "yes" }
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "atm"})), ());
  }

  // "NO" tag test.
  {
    Tags const tags = {
      { "building", "yes" },
      { "shop", "no" },
      { "atm", "no" }
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"building"})), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Capital)
{
  {
    Tags const tags = {
      { "capital", "6" },
      { "place", "city" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"place", "city", "capital", "6"})), (params));
  }

  {
    Tags const tags = {
      { "admin_level", "6" },
      { "capital", "no" },
      { "place", "city" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"place", "city"})), ());
  }

  {
    Tags const tags = {
      {"boundary", "administrative"},
      {"capital", "2"},
      {"place", "city"},
      {"admin_level", "4"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"place", "city", "capital", "2"})), (params));
    TEST(params.IsTypeExist(GetType({"boundary", "administrative", "4"})), (params));
    TEST(params.IsTypeExist(GetType({"place", "city", "capital", "4"})), (params));
  }

  {
    Tags const tags = {
      {"capital", "yes"},
      {"place", "town"},
      {"admin_level", "7"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"place", "city", "capital", "2"})), (params));
    TEST(params.IsTypeExist(GetType({"place", "city", "capital", "7"})), (params));
  }

  {
    Tags const tags = {
      {"capital", "yes"},
      {"admin_level", "7"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 0, (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_DePlace)
{
  {
    Tags const tags = {
      {"de:place", "town"},
      {"name", "xyz"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"place", "town"})), (params));
    TEST(!params.IsEmptyNames(), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Route)
{
  {
    Tags const tags = {
      { "highway", "motorway" },
      { "ref", "I 95" }
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({ "highway", "motorway" })), ());
    TEST_EQUAL(params.ref, "I 95", ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Layer)
{
  {
    Tags const tags = {
      { "highway", "motorway" },
      { "bridge", "yes" },
      { "layer", "2" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "motorway", "bridge"})), ());
    TEST_EQUAL(params.layer, 2, ());
  }

  {
    Tags const tags = {
      { "highway", "trunk" },
      { "tunnel", "yes" },
      { "layer", "-1" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "trunk", "tunnel"})), ());
    TEST_EQUAL(params.layer, -1, ());
  }

  {
    Tags const tags = {
      { "highway", "secondary" },
      { "bridge", "yes" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "secondary", "bridge"})), ());
    TEST_EQUAL(params.layer, 1, ());
  }

  /* TODO: add an explicit layer=1 for ANY bridge/tunnel value except "no"
  {
    Tags const tags = {
      { "highway", "secondary" },
      { "bridge", "positive_value" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "secondary", "bridge"})), ());
    TEST_EQUAL(params.layer, 1, ());
  }
  */

  {
    Tags const tags = {
      { "highway", "primary" },
      { "tunnel", "yes" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "primary", "tunnel"})), ());
    TEST_EQUAL(params.layer, -1, ());
  }

  {
    Tags const tags = {
      { "highway", "living_street" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "living_street"})), ());
    TEST_EQUAL(params.layer, 0, ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Amenity)
{
  {
    Tags const tags = {
      { "amenity", "bbq" },
      { "fuel", "wood" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "bbq"})), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Hwtag)
{
  {
    Tags const tags = {
      { "railway", "light_rail" },
      { "access", "private" },
      { "oneway", "true" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "light_rail"})), ());
  }

  {
    Tags const tags = {
        {"oneway", "-1"},
        {"highway", "primary"},
        {"access", "private"},
        {"lit", "no"},
        {"foot", "no"},
        {"bicycle", "yes"},
        {"oneway:bicycle", "no"},
        {"motor_vehicle", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 6, (params));
    TEST(params.IsTypeExist(GetType({"highway", "primary"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "oneway"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "private"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nofoot"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesbicycle"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "bidir_bicycle"})), ());
    // We don't put yescar tag for features that already Yes by default.
    //TEST(params.IsTypeExist(GetType({"hwtag", "yescar"})), ());
  }

  {
    Tags const tags = {
        {"foot", "designated"},
        {"cycleway", "lane"},
        {"highway", "primary"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"highway", "primary"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesfoot"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesbicycle"})), ());
  }

  {
    Tags const tags = {
        {"foot", "use_sidepath"},
        {"sidewalk", "left"},
        {"cycleway:both", "separate"},
        {"highway", "primary"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"highway", "primary"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nofoot"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nocycleway"})), ());
    // No cycleway doesn't mean that bicycle is not allowed.
    //TEST(params.IsTypeExist(GetType({"hwtag", "nobicycle"})), ());
  }

  {
    Tags const tags = {
        {"foot", "unknown"},
        {"bicycle", "dismount"},
        {"highway", "bridleway"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "bridleway"})), ());
  }

  {
    Tags const tags = {
        {"motor_vehicle", "yes"},
        {"motorcar", "no"},
        {"highway", "track"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"highway", "track"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nocar"})), ());
  }

  {
    Tags const tags = {
        {"foot", "no"},
        {"bicycle", "no"},
        {"sidewalk:left", "yes"},
        {"cycleway:right", "yes"},
        {"highway", "trunk"},
        {"motorcar", "designated"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"highway", "trunk"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nofoot"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nobicycle"})), ());
    //TEST(params.IsTypeExist(GetType({"hwtag", "yescar"})), ());
  }

  {
    Tags const tags = {
        {"foot", "yes"},
        {"bicycle", "yes"},
        {"sidewalk", "no"},
        {"cycleway", "no"},
        {"highway", "path"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"highway", "path"})), (params));
    TEST(params.IsTypeExist(GetType({"hwtag", "yesfoot"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesbicycle"})), ());
  }

  {
    Tags const tags = {
        {"sidewalk:both", "no"},
        {"bicycle_road", "yes"},
        {"highway", "residential"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"highway", "residential"})), (params));
    TEST(params.IsTypeExist(GetType({"hwtag", "nosidewalk"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesbicycle"})), ());
  }

  {
    Tags const tags = {
      {"bench", "yes"},
      {"bicycle", "yes"},
      {"bin", "yes"},
      {"foot", "designated"},
      {"highway", "footway"},
      {"lit", "yes"},
      {"public_transport", "platform"},
      {"railway", "platform"},
      {"shelter", "yes"},
      {"smoothness", "good"},
      {"surface", "paving_stones"},
      {"tactile_paving", "yes"},
      {"traffic_sign", "DE:239,DE:1022-10"},
      {"tram", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 8, (params));
    TEST(params.IsTypeExist(GetType({"highway", "footway"})), (params));
    TEST(params.IsTypeExist(GetType({"hwtag", "yesbicycle"})), ());
    TEST(!params.IsTypeExist(GetType({"hwtag", "yesfoot"})), ());

    /// @todo One platform is enough.
    TEST(params.IsTypeExist(GetType({"railway", "platform"})), (params));
    TEST(params.IsTypeExist(GetType({"public_transport", "platform"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Surface)
{
  TestSurfaceTypes("asphalt", "excellent", "", "paved_good");
  TestSurfaceTypes("asphalt", "", "", "paved_good");
  TestSurfaceTypes("asphalt", "intermediate", "", "paved_bad");
  TestSurfaceTypes("asphalt", "bad", "", "paved_bad");
  TestSurfaceTypes("asphalt", "", "0", "paved_bad");

  TestSurfaceTypes("cobblestone", "good", "", "paved_good");
  TestSurfaceTypes("cobblestone", "", "", "paved_bad");
  TestSurfaceTypes("cobblestone", "intermediate", "", "paved_bad");
  TestSurfaceTypes("cobblestone", "very_bad", "", "paved_bad");

  TestSurfaceTypes("compacted", "good", "", "unpaved_good");
  TestSurfaceTypes("compacted", "", "", "unpaved_good");
  TestSurfaceTypes("fine_gravel", "", "", "unpaved_good");
  TestSurfaceTypes("fine_gravel", "intermediate", "", "unpaved_good");
  TestSurfaceTypes("pebblestone", "bad", "", "unpaved_bad");
  TestSurfaceTypes("pebblestone", "horrible", "", "unpaved_bad");

  TestSurfaceTypes("gravel", "excellent", "", "unpaved_good");
  TestSurfaceTypes("gravel", "good", "", "unpaved_good");
  TestSurfaceTypes("gravel", "", "", "unpaved_good");
  TestSurfaceTypes("gravel", "", "1.5", "unpaved_bad");
  TestSurfaceTypes("gravel", "intermediate", "", "unpaved_bad");
  TestSurfaceTypes("gravel", "bad", "", "unpaved_bad");
  TestSurfaceTypes("gravel", "very_bad", "", "unpaved_bad");

  TestSurfaceTypes("paved", "", "", "paved_good");
  TestSurfaceTypes("paved", "", "2", "paved_good");
  TestSurfaceTypes("paved", "intermediate", "", "paved_bad");
  TestSurfaceTypes("", "excellent", "", "paved_good");
  TestSurfaceTypes("", "intermediate", "", "paved_bad");
  TestSurfaceTypes("wood", "good", "", "paved_good");
  TestSurfaceTypes("wood", "", "3", "paved_good");
  TestSurfaceTypes("wood", "", "", "paved_bad");

  TestSurfaceTypes("pebblestone", "", "4", "unpaved_good");
  TestSurfaceTypes("pebblestone", "", "", "unpaved_good");
  TestSurfaceTypes("unpaved", "", "2", "unpaved_good");
  TestSurfaceTypes("unpaved", "", "", "unpaved_good");
  TestSurfaceTypes("unpaved", "intermediate", "", "unpaved_bad");
  TestSurfaceTypes("unpaved", "bad", "", "unpaved_bad");

  TestSurfaceTypes("ground", "good", "2", "unpaved_good");
  TestSurfaceTypes("ground", "", "5", "unpaved_good");
  TestSurfaceTypes("ground", "", "3", "unpaved_good");
  TestSurfaceTypes("ground", "", "2.5", "unpaved_bad");
  TestSurfaceTypes("ground", "", "", "unpaved_bad");
  TestSurfaceTypes("ground", "", "1", "unpaved_bad");
  TestSurfaceTypes("ground", "intermediate", "", "unpaved_bad");
  TestSurfaceTypes("ground", "bad", "", "unpaved_bad");
  TestSurfaceTypes("mud", "good", "1", "unpaved_good");
  TestSurfaceTypes("mud", "", "3", "unpaved_good");
  TestSurfaceTypes("mud", "", "", "unpaved_bad");

  TestSurfaceTypes("", "bad", "", "paved_bad");
  TestSurfaceTypes("", "unknown", "", "paved_bad");
  TestSurfaceTypes("", "horrible", "", "unpaved_bad");
  TestSurfaceTypes("unknown", "", "", "unpaved_good");
  TestSurfaceTypes("unknown", "unknown", "", "unpaved_good");

  TestSurfaceTypes("asphalt;concrete", "", "", "paved_good");
  TestSurfaceTypes("concrete:plates", "", "", "paved_good");
  TestSurfaceTypes("cobblestone:flattened", "", "", "paved_bad");
  TestSurfaceTypes("dirt/sand", "", "", "unpaved_bad");

  {
    Tags const tags = {
        {"highway", "trunk"},
        {"smoothness", "intermediate"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "trunk"})), (params));
  }

  {
    Tags const tags = {
        {"highway", "motorway"},
        {"smoothness", "intermediate"},
        {"surface", "asphalt"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"highway", "motorway"})), (params));
  }

  {
    Tags const tags = {
        {"highway", "track"},
        {"smoothness", "bad"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"highway", "track"})), (params));
    TEST(params.IsTypeExist(GetType({"psurface", "unpaved_bad"})), (params));
  }

  {
    Tags const tags = {
      {"highway", "track"},
      {"tracktype", "grade1"},
      {"smoothness", "intermediate"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"highway", "track"})), (params));
    TEST(params.IsTypeExist(GetType({"psurface", "paved_bad"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Ferry)
{
  uint32_t const ferryType = GetType({"route", "ferry"});
  TEST(routing::PedestrianModel::AllLimitsInstance().IsRoadType(ferryType), ());
  TEST(routing::BicycleModel::AllLimitsInstance().IsRoadType(ferryType), ());
  TEST(routing::CarModel::AllLimitsInstance().IsRoadType(ferryType), ());

  auto const yesCar = GetType({"hwtag", "yescar"});
  auto const noFoot = GetType({"hwtag", "nofoot"});
  auto const yesBicycle = GetType({"hwtag", "yesbicycle"});

  {
    Tags const tags = {
      { "route", "ferry" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(ferryType), (params));
    TEST(params.IsTypeExist(GetType({"hwtag", "nocar"})), ());
  }

  uint32_t const shuttleType = GetType({"route", "shuttle_train"});
  /// @todo Strange, but they are processed by foot/bicycle=yes/no in VehicleModel.
  //TEST(routing::PedestrianModel::AllLimitsInstance().IsRoadType(shuttleType), ());
  //TEST(routing::BicycleModel::AllLimitsInstance().IsRoadType(shuttleType), ());
  TEST(routing::CarModel::AllLimitsInstance().IsRoadType(shuttleType), ());

  {
    Tags const tags = {
      { "route", "shuttle_train" },
      { "bicycle", "yes" },
      { "foot", "no" },
      { "motorcar", "yes" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 4, (params));
    TEST(params.IsTypeExist(shuttleType), (params));
    TEST(params.IsTypeExist(yesBicycle), (params));
    TEST(params.IsTypeExist(noFoot), (params));
    TEST(params.IsTypeExist(yesCar), (params));
  }

  {
    Tags const tags = {
      { "route", "train" },
      { "shuttle", "yes" },
      { "motor_vehicle", "yes" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(shuttleType), (params));
    TEST(params.IsTypeExist(yesCar), (params));
  }

  {
    Tags const tags = {
      { "route", "train" },
      { "shuttle", "no" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 0, (params));
  }

  {
    Tags const tags = {
      { "foot", "no" },
      { "bicycle", "no" },
      { "motorcar", "yes" },
      { "route", "ferry" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 4, (params));
    TEST(params.IsTypeExist(ferryType), (params));
    TEST(params.IsTypeExist(yesCar), ());
    TEST(params.IsTypeExist(noFoot), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nobicycle"})), ());
  }

  {
    Tags const tags = {
        {"ferry", "path"},
        {"bicycle", "no"},
        {"route", "ferry"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    // - Existing ferry=path forces to set bicycle/foot=yes. Finally, bicycle=yes prevails on bicycle=no.
    // Only one way like this in all OSM: https://www.openstreetmap.org/way/913507515
    // - Assume nocar for ferries by default, unless otherwise specified
    TEST_EQUAL(params.m_types.size(), 4, (params));
    TEST(params.IsTypeExist(GetType({"route", "ferry"})), ());
    TEST(params.IsTypeExist(yesBicycle), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesfoot"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nocar"})), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Boundary)
{
  {
    Tags const tags = {
      { "admin_level", "4" },
      { "boundary", "administrative" },
      { "admin_level", "2" },
      { "boundary", "administrative" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"boundary", "administrative", "2"})), ());
    TEST(params.IsTypeExist(GetType({"boundary", "administrative", "4"})), ());
  }

  {
    Tags const tags = {
      { "protect_class", "1b" },
      { "boundary", "protected_area" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"boundary", "protected_area", "1"})), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Dibrugarh)
{
  Tags const tags = {
    { "AND_a_c", "10001373" },
    { "addr:city", "Dibrugarh" },
    { "addr:housenumber", "hotel vishal" },
    { "addr:postcode", "786001" },
    { "addr:street", "Marwari Patty,Puja Ghat" },
    { "name", "Dibrugarh" },
    { "phone", "03732320016" },
    { "place", "city" },
    { "website", "http://www.hotelvishal.in" },
  };

  auto const params = GetFeatureBuilderParams(tags);

  TEST_EQUAL(params.m_types.size(), 1, (params));
  TEST(params.IsTypeExist(GetType({"place", "city"})), (params));
  std::string_view name;
  TEST(params.name.GetString(StringUtf8Multilang::kDefaultCode, name), (params));
  TEST_EQUAL(name, "Dibrugarh", (params));
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Subway)
{
  {
    Tags const tags = {
      { "network", "Московский метрополитен" },
      { "operator", "ГУП «Московский метрополитен»" },
      { "railway", "station" },
      { "station", "subway" },
      { "transport", "subway" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "station", "subway", "moscow"})), (params));
  }

  {
    Tags const tags = {
      {"name", "14th Street-8th Avenue (A,C,E,L)"},
      {"network", "New York City Subway"},
      {"railway", "station"},
      {"wheelchair", "yes"},
      {"transport", "subway"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"railway", "station", "subway", "newyork"})), (params));
  }

  {
    Tags const tags = {
      { "name", "S Landsberger Allee" },
      { "phone", "030 29743333" },
      { "public_transport", "stop_position" },
      { "railway", "station" },
      { "network", "New York City Subway" },
      { "station", "light_rail" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "station", "light_rail"})), (params));
  }

  {
    Tags const tags = {
      { "monorail", "yes" },
      { "name", "Улица Академика Королёва" },
      { "network", "Московский метрополитен" },
      { "operator", "ГУП «Московский метрополитен»" },
      { "public_transport", "stop_position" },
      { "railway", "station" },
      { "station", "monorail" },
      { "transport", "monorail" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "station", "monorail"})), (params));
  }

  {
    Tags const tags = {
      { "line",	"Northern, Bakerloo" },
      { "name",	"Charing Cross" },
      { "network", "London Underground" },
      { "operator", "TfL" },
      { "railway", "station" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "station", "subway", "london"})), (params));
  }

  {
    Tags const tags = {
      { "artist_name", "Архитекторы: Н.Лопато, В.Шкарупин, В.Телепнев Скульптор: Л.Зильбер. Дизайнер: Н.Грибов" },
      { "colour", "red" },
      { "int_name", "Frunzienskaja" },
      { "name", "Фрунзенская" },
      { "name:be", "Фрунзенская" },
      { "name:en", "Frunzienskaja" },
      { "name:ru", "Фрунзенская" },
      { "network", "Минский метрополитен" },
      { "operator", "КУП «Минский метрополитен»" },
      { "public_transport", "stop_position" },
      { "railway", "station" },
      { "ref", "218" },
      { "start_date", "1990-12-31" },
      { "station", "subway" },
      { "subway", "yes" },
      { "transport", "subway" },
      { "website", "https://metropoliten.by/sxema-linii/54/224/" },
      { "wheelchair", "no" },
      { "wikidata", "Q2445504" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"wheelchair", "no"})), (params));
    TEST(params.IsTypeExist(GetType({"railway", "station", "subway", "minsk"})), (params));
  }

  {
    Tags const tags = {
      { "artist_name", "Архитектор: В. Лопато" },
      { "colour", "green" },
      { "int_name", "Jubiliejnaja plošča" },
      { "layer", "-5" },
      { "line", "Зеленый Луг" },
      { "name", "Юбілейная плошча" },
      { "name:be", "Юбілейная плошча" },
      { "name:ru", "Юбилейная площадь" },
      { "network", "Минский метрополитен" },
      { "operator", "КУП «Минский метрополитен»" },
      { "public_transport", "stop_position" },
      { "railway", "station" },
      { "ref", "316" },
      { "start_date", "2020-11-06" },
      { "station", "subway" },
      { "subway", "yes" },
      { "transport", "subway" },
      { "website", "https://metropoliten.by/sxema-linii/91/2781/" },
      { "wikidata", "Q4365831" },
      { "wikipedia", "be:Юбілейная плошча (станцыя метро)" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "station", "subway", "minsk"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_PublicTransport)
{
  {
    Tags const tags = {
      { "name", "Платонава" },
      { "public_transport", "stop_position" },
      { "tram", "yes" },
    };

    auto const params = GetFeatureBuilderParams(tags, OsmElement::EntityType::Node);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "tram_stop"})), (params));
  }

  {
    Tags const tags = {
      { "funicular", "yes" },
      { "name", "Gare Pfaffenthal-Kirchberg" },
      { "public_transport", "stop_position" },
    };

    auto const params = GetFeatureBuilderParams(tags, OsmElement::EntityType::Node);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "station", "funicular"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Hospital)
{
  {
    Tags const tags = {
      { "building", "hospital" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"building"})), (params));
  }

  {
    Tags const tags = {
      { "building", "yes" },
      { "amenity", "hospital" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"building"})), (params));
    TEST(params.IsTypeExist(GetType({"amenity", "hospital"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Entrance)
{
  {
    Tags const tags = {
      { "building", "entrance" },
      { "barrier", "entrance" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"entrance"})), (params));
    TEST(params.IsTypeExist(GetType({"barrier", "entrance"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Moscow)
{
  {
    Tags const tags = {
      { "addr:country", "RU" },
      { "addr:region", "Москва" },
      { "admin_level", "2" },
      { "alt_name:vi", "Mạc Tư Khoa" },
      { "capital", "yes" },
      { "ele", "156" },
      { "int_name", "Moscow" },
      { "is_capital", "country" },
      { "ISO3166-2", "RU-MOW" },
      { "name", "Москва" },
      { "note", "эта точка должна быть здесь, в историческом центре Москвы" },
      { "official_status", "ru:город" },
      { "okato:user", "none" },
      { "place", "city" },
      { "population", "12108257" },
      { "population:date", "2014-01-01" },
      { "rank", "0" },
      { "wikipedia", "ru:Москва" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"place", "city", "capital", "2"})), (params));
    TEST(170 <= params.rank && params.rank <= 180, (params));
    TEST(!params.name.IsEmpty(), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Translations)
{
  Tags const tags = {
    { "name", "Paris" },
    { "name:ru", "Париж" },
    { "name:en", "Paris" },
    { "name:en:pronunciation", "ˈpæɹ.ɪs" },
    { "name:fr:pronunciation", "paʁi" },
    { "place", "city" },
    { "population", "2243833" }
  };

  auto const params = GetFeatureBuilderParams(tags);

  TEST_EQUAL(params.m_types.size(), 1, (params));
  TEST(params.IsTypeExist(GetType({"place", "city"})), ());

  std::string_view name;
  TEST(params.name.GetString(StringUtf8Multilang::kDefaultCode, name), (params));
  TEST_EQUAL(name, "Paris", (params));
  TEST(params.name.GetString(StringUtf8Multilang::kEnglishCode, name), (params));
  TEST_EQUAL(name, "Paris", (params));
  TEST(!params.name.GetString("fr", name), (params));
  TEST(params.name.GetString("ru", name), (params));
  TEST_EQUAL(name, "Париж", (params));
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Cuisine)
{
  {
    Tags const tags = {
      { "cuisine", "indian ; steak,coffee  shop " },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"cuisine", "indian"})), (params));
    TEST(params.IsTypeExist(GetType({"cuisine", "steak_house"})), (params));
    TEST(params.IsTypeExist(GetType({"cuisine", "coffee_shop"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Hotel)
{
  using Type = std::vector<std::string>;
  std::vector<std::pair<std::vector<Type>, Tags>> const types = {
    {
      {{"tourism", "hotel"}},
      {{"tourism", "hotel"}},
    },
    {
      {{"building"}},
      {{"building", "hotel"}},
    },
    {
      {{"tourism", "hotel"}},
      {{"hotel", "yes"}},
    }
  };

  for (auto const & t : types)
  {
    auto const params = GetFeatureBuilderParams(t.second);
    TEST_EQUAL(t.first.size(), params.m_types.size(), (params, t));
    for (auto const & t : t.first)
      TEST(params.IsTypeExist(GetType(t)), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_OldName)
{
  {
    Tags const tags = {
      {"highway", "residential"},
      {"name", "Улица Веткина"},
      {"old_name", "Царская Ветка"}
    };

    auto const params = GetFeatureBuilderParams(tags);

    std::string_view s;
    params.name.GetString(StringUtf8Multilang::kDefaultCode, s);
    TEST_EQUAL(s, "Улица Веткина", ());
    params.name.GetString(StringUtf8Multilang::GetLangIndex("old_name"), s);
    TEST_EQUAL(s, "Царская Ветка", ());
  }
  {
    Tags const tags = {{"place", "city"},
                       {"name", "Санкт-Петербург"},
                       {"old_name:1914-08-31--1924-01-26", "Петроград"},
                       {"old_name:1924-01-26--1991-09-06", "Ленинград"},
                       {"old_name:en:1914-08-31--1924-01-26", "Petrograd"},
                       {"old_name:en:1924-01-26--1991-09-06", "Leningrad"},
                       {"old_name:fr", "Pétrograd;Léningrad"}};

    auto const params = GetFeatureBuilderParams(tags);

    std::string_view s;
    params.name.GetString(StringUtf8Multilang::kDefaultCode, s);
    TEST_EQUAL(s, "Санкт-Петербург", ());
    params.name.GetString(StringUtf8Multilang::GetLangIndex("old_name"), s);
    // We ignore old_name:lang and old_name:lang:dates but support old_name:dates.
    TEST_EQUAL(s, "Петроград;Ленинград", ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_AltName)
{
  {
    Tags const tags = {
      {"tourism", "museum"},
      {"name", "Московский музей современного искусства"},
      {"alt_name", "MMOMA"}
    };

    auto const params = GetFeatureBuilderParams(tags);

    std::string_view s;
    params.name.GetString(StringUtf8Multilang::kDefaultCode, s);
    TEST_EQUAL(s, "Московский музей современного искусства", ());
    params.name.GetString(StringUtf8Multilang::GetLangIndex("alt_name"), s);
    TEST_EQUAL(s, "MMOMA", ());
  }
  {
    Tags const tags = {
      {"tourism", "museum"},
      {"name", "Московский музей современного искусства"},
      {"alt_name:en", "MMOMA"}
    };

    auto const params = GetFeatureBuilderParams(tags);

    std::string_view s;
    params.name.GetString(StringUtf8Multilang::kDefaultCode, s);
    TEST_EQUAL(s, "Московский музей современного искусства", ());
    // We do not support alt_name:lang.
    TEST(!params.name.GetString(StringUtf8Multilang::GetLangIndex("alt_name"), s), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_NameJaKana)
{
  {
    Tags const tags = {
      {"place", "city"},
      {"name", "Tokyo"},
      {"name:ja_kana", "トウキョウト"}
    };

    auto const params = GetFeatureBuilderParams(tags);

    std::string_view s;
    params.name.GetString(StringUtf8Multilang::kDefaultCode, s);
    TEST_EQUAL(s, "Tokyo", ());
    params.name.GetString(StringUtf8Multilang::GetLangIndex("ja_kana"), s);
    TEST_EQUAL(s, "トウキョウト", ());
  }
  {
    Tags const tags = {
      {"place", "city"},
      {"name", "Tokyo"},
      {"name:ja-Hira", "とうきょうと"}
    };

    auto const params = GetFeatureBuilderParams(tags);

    std::string_view s;
    params.name.GetString(StringUtf8Multilang::kDefaultCode, s);
    TEST_EQUAL(s, "Tokyo", ());
    // Save ja-Hira as ja_kana if there is no ja_kana.
    params.name.GetString(StringUtf8Multilang::GetLangIndex("ja_kana"), s);
    TEST_EQUAL(s, "とうきょうと", ());
  }
  {
    Tags const tags = {
      {"place", "city"},
      {"name", "Tokyo"},
      {"name:ja_kana", "トウキョウト"},
      {"name:ja-Hira", "とうきょうと"}
    };

    auto const params = GetFeatureBuilderParams(tags);

    std::string_view s;
    params.name.GetString(StringUtf8Multilang::kDefaultCode, s);
    TEST_EQUAL(s, "Tokyo", ());
    // Prefer ja_kana over ja-Hira. ja_kana tag goes first.
    params.name.GetString(StringUtf8Multilang::GetLangIndex("ja_kana"), s);
    TEST_EQUAL(s, "トウキョウト", ());
  }
  {
    Tags const tags = {
      {"place", "city"},
      {"name", "Tokyo"},
      {"name:ja-Hira", "とうきょうと"},
      {"name:ja_kana", "トウキョウト"}
    };

    auto const params = GetFeatureBuilderParams(tags);

    std::string_view s;
    params.name.GetString(StringUtf8Multilang::kDefaultCode, s);
    TEST_EQUAL(s, "Tokyo", ());
    // Prefer ja_kana over ja-Hira. ja-Hira tag goes first.
    params.name.GetString(StringUtf8Multilang::GetLangIndex("ja_kana"), s);
    TEST_EQUAL(s, "トウキョウト", ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_MergeTags)
{
  {
    Tags const tags = {
        {"amenity", "parking"},
        {"parking", "multi-storey"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "parking", "multi-storey"})), (params));
  }
  {
    Tags const tags = {
        {"amenity", "parking"},
        {"location", "underground"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "parking", "underground"})), (params));
  }
  {
    Tags const tags = {
        {"amenity", "parking_space"},
        {"parking", "underground"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "parking_space", "underground"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_ReuseTags)
{
  {
    Tags const tags = {
        {"amenity", "parking"},
        {"access", "private"},
        {"fee", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "parking", "private"})), (params));
    TEST(params.IsTypeExist(GetType({"amenity", "parking", "fee"})), (params));
    TEST(params.IsTypeExist(GetType({"fee", "yes"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_DoNotMergeTags)
{
  {
    Tags const tags = {
        {"place", "unknown_place_value"},
        {"country", "unknown_country_value"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 0, (params));
  }
  {
    Tags const tags = {
        {"amenity", "hospital"},
        {"emergency", "yes"},
        {"phone", "77777777"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "hospital"})), (params));
    TEST(!params.IsTypeExist(GetType({"emergency", "phone"})), (params));
  }
  {
    Tags const tags = {
        {"shop", "unknown_shop_value"},
        {"photo", "photo_url"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"shop"})), (params));
    TEST(!params.IsTypeExist(GetType({"shop", "photo"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_AerodromeType)
{
  Tags const tags = {
      {"aeroway", "aerodrome"},
      {"aerodrome:type", "international ; public"},
  };

  auto const params = GetFeatureBuilderParams(tags);

  TEST_EQUAL(params.m_types.size(), 1, (params));
  TEST(params.IsTypeExist(GetType({"aeroway", "aerodrome", "international"})), (params));
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_CuisineType)
{
  {
    // Collapce synonyms to single type.
    Tags const tags = {
        {"cuisine", "BBQ ; barbeque, barbecue;bbq"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"cuisine", "barbecue"})), (params));
  }
  {
    // Replace space with underscore, ignore commas and semicolons.
    Tags const tags = {
        {"cuisine", ",; ;   ; Fish and  Chips;; , , ;"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"cuisine", "fish_and_chips"})), (params));
  }
  {
    // Multiple cuisines.
    Tags const tags = {
        {"cuisine", "Italian Pizza , mediterranean;international,"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"cuisine", "italian_pizza"})), (params));
    TEST(params.IsTypeExist(GetType({"cuisine", "mediterranean"})), (params));
    TEST(params.IsTypeExist(GetType({"cuisine", "international"})), (params));
  }
}

/// @todo We don't have drawing rules for 'noexit' now.
/*
UNIT_CLASS_TEST(TestWithClassificator, OsmType_NoExit)
{
  {
    Tags const tags = { {"noexit", "yes" } };
    auto const params = GetFeatureBuilderParams(tags);
    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"noexit"})), (params));
  }

  {
    Tags const tags = { {"noexit", "motor_vehicle" } };
    auto const params = GetFeatureBuilderParams(tags);
    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"noexit"})), (params));
  }

  {
    Tags const tags = { {"noexit", "no" } };
    auto const params = GetFeatureBuilderParams(tags);
    TEST_EQUAL(params.m_types.size(), 0, (params));
  }
}
*/

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Junctions)
{
  for (char const * value : { "yes", "jughandle" })
  {
    Tags const tags = {
        {"junction", value }
    };

    // Useless now, because they don't have any rules and are not set as an exception.
    auto const params = GetFeatureBuilderParams(tags);
    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"junction"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Recycling)
{
  {
    Tags const tags = {
      {"amenity", "recycling" },
      {"recycling:glass_bottles", "yes"},
      {"recycling:green_waste", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "recycling"})), (params));
    TEST(params.IsTypeExist(GetType({"recycling", "glass_bottles"})), (params));
    TEST(params.IsTypeExist(GetType({"recycling", "green_waste"})), (params));
  }

  {
    Tags const tags = {
      {"amenity", "recycling" },
      {"recycling_type", "centre"},
      {"recycling:garden_waste", "no"},
      {"recycling:organic", "no"},
      {"recycling:glass", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "recycling", "centre"})), (params));
    TEST(params.IsTypeExist(GetType({"recycling", "glass_bottles"})), (params));
  }

  {
    Tags const tags = {
      {"amenity", "recycling" },
      {"recycling_type", "container"},
      {"recycling:metal", "yes"},
      {"recycling:batteries", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "recycling", "container"})), (params));
    TEST(params.IsTypeExist(GetType({"recycling", "scrap_metal"})), (params));
    TEST(params.IsTypeExist(GetType({"recycling", "batteries"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Metadata)
{
  auto const getDescr = [](FeatureBuilderParams const & params, std::string_view lang)
  {
    std::string buffer(params.GetMetadata().Get(feature::Metadata::FMD_DESCRIPTION));
    TEST(!buffer.empty(), ());
    auto const mlStr = StringUtf8Multilang::FromBuffer(std::move(buffer));

    std::string_view desc;
    mlStr.GetString(StringUtf8Multilang::GetLangIndex(lang), desc);
    return std::string(desc);
  };

  {
    Tags const tags = {
      {"amenity", "restaurant" },
      {"description:ru", "Хорошие настойки"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "restaurant"})), (params));
    TEST_EQUAL(getDescr(params, "ru"), "Хорошие настойки", ());
  }

  {
    Tags const tags = {
      {"amenity", "atm" },
      {"operator", "Default"},
      {"operator:en", "English"},
      {"brand::kk", "KK language"},
      {"brand:en", "English"},
      {"description", "Default"},
      {"description::kk", "KK language"},
    };

    auto const params = GetFeatureBuilderParams(tags);
    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "atm"})), (params));
    TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_OPERATOR), "Default", ());
    TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_BRAND), "English", ());
    TEST_EQUAL(getDescr(params, "default"), "Default", ());
  }

  {
    Tags const tags = {
      {"amenity", "cafe"},
      {"internet_access", "wlan"},
      {"internet_access:password", "corrientes4199"},
      {"name", "Jimbo"},
      {"wifi", "corrientes4199"},
    };

    auto const params = GetFeatureBuilderParams(tags);
    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "cafe"})), (params));
    TEST(params.IsTypeExist(GetType({"internet_access", "wlan"})), (params));
    TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_INTERNET), "wlan", ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Vending)
{
  {
    Tags const tags = {
      {"amenity", "vending_machine" },
      {"vending", "parcel_pickup;parcel_mail_in"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "parcel_locker"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Cliff)
{
  {
    Tags const tags = {
      {"cliff", "yes"},
      {"natural", "ridge" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST(params.IsTypeExist(GetType({"natural", "cliff"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Organic)
{
  {
    Tags const tags = {
      {"organic", "only"},
      {"amenity", "cafe" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "cafe"})), (params));
    TEST(params.IsTypeExist(GetType({"organic", "only"})), (params));
  }

  {
    Tags const tags = {
      {"organic", "no"},
      {"shop", "bakery" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"shop", "bakery"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Internet)
{
  {
    Tags const tags = {
      {"internet_access", "no"},
      {"wifi", "no"},
      {"amenity", "cafe"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "cafe"})), (params));
  }

  {
    Tags const tags = {
      {"internet_access", "wlan"},
      {"office", "it"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"office"})), (params));
    TEST(params.IsTypeExist(GetType({"internet_access", "wlan"})), (params));
  }

  {
    Tags const tags = {
      {"wifi", "free"},
      {"internet_access", "yes"},
      {"shop", "clothes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"shop", "clothes" })), (params));
    TEST(params.IsTypeExist(GetType({"internet_access", "wlan"})), (params));
  }

  {
    Tags const tags = {
      {"wifi", "no"},
      {"internet_access", "terminal"},
      {"amenity", "internet_cafe"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "internet_cafe" })), (params));
    TEST(params.IsTypeExist(GetType({"internet_access"})), (params));
  }
}

// Significant military danger areas for DMZ like in Cyprus or Korea.
UNIT_CLASS_TEST(TestWithClassificator, OsmType_MilitaryDanger)
{
  {
    Tags const tags = {
      {"landuse", "military"},
      {"military", "danger_area"},
      {"wikipedia", "xxx"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"landuse", "military", "danger_area"})), (params));
  }

  {
    Tags const tags = {
      {"landuse", "military"},
      {"military", "cordon"},
      {"wikipedia", "xxx"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"landuse", "military", "danger_area"})), (params));
  }

  {
    Tags const tags = {
      {"landuse", "military"},
      {"military", "danger_area"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    // Skip danger_area type without additional wikipedia tags.
    TEST(params.IsTypeExist(GetType({"landuse", "military"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_ChargingStation)
{
  {
    Tags const tags = {
      {"amenity", "charging_station"},
      {"motorcar", "no"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "charging_station"})), (params));
  }

  {
    Tags const tags = {
      {"amenity", "charging_station"},
      {"bicycle", "yes"},
      {"motorcar", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "charging_station", "bicycle"})), (params));
    TEST(params.IsTypeExist(GetType({"amenity", "charging_station", "motorcar"})), (params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_RailwayRail)
{
  using Type = std::vector<std::string>;
  std::vector<std::pair<Type, Tags>> const railTypes = {
    {{"railway", "rail", "highspeed"}, {{"railway", "rail"}, {"highspeed", "positive_value"}}},
    {{"railway", "rail", "highspeed"}, {{"railway", "rail"}, {"usage", "main"}, {"highspeed", "positive_value"}}},
    {{"railway", "rail", "tourism"}, {{"railway", "rail"}, {"usage", "tourism"}}},
    {{"railway", "rail", "main"}, {{"railway", "rail"}, {"usage", "main"}}},
    {{"railway", "rail", "branch"}, {{"railway", "rail"}, {"usage", "branch"}}},
    {{"railway", "rail", "branch"}, {{"railway", "rail"}}},
    {{"railway", "rail", "utility"}, {{"railway", "rail"}, {"usage", "military"}}},
    {{"railway", "rail", "utility"}, {{"railway", "rail"}, {"usage", "industrial"}, {"service", "spur"}}},
    {{"railway", "rail", "spur"}, {{"railway", "rail"}, {"service", "spur"}}},
    {{"railway", "rail", "service"}, {{"railway", "rail"}, {"service", "siding"}}},
    {{"railway", "rail", "service"}, {{"railway", "rail"}, {"highspeed", "positive_value"}, {"service", "siding"}}},
    {{"railway", "rail", "service"}, {{"railway", "rail"}, {"usage", "main"}, {"service", "siding"}}},
    {{"railway", "rail", "service"}, {{"railway", "rail"}, {"usage", "branch"}, {"service", "yard"}}},
    {{"railway", "rail", "service"}, {{"railway", "rail"}, {"usage", "unsupported_value"}, {"service", "crossover"}}},
    // TODO: better match to railway-rail-spur:
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "unsupported_value"}}},
    // TODO: better match following 3 cases to railway-rail-service:
    {{"railway", "rail"}, {{"railway", "rail"}, {"service", "unsupported_value"}}},
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "main"}, {"service", "unsupported_value"}}},
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "unsupported_value"}, {"service", "unsupported_value"}}},

    // Bridges (note, railway-rail-bridge should be never matched).
    {{"railway", "rail", "highspeed", "bridge"}, {{"railway", "rail"}, {"highspeed", "positive_value"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "highspeed", "bridge"}, {{"railway", "rail"}, {"usage", "main"}, {"highspeed", "positive_value"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "tourism", "bridge"}, {{"railway", "rail"}, {"usage", "tourism"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "main", "bridge"}, {{"railway", "rail"}, {"usage", "main"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "branch", "bridge"}, {{"railway", "rail"}, {"usage", "branch"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "branch", "bridge"}, {{"railway", "rail"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "utility", "bridge"}, {{"railway", "rail"}, {"usage", "industrial"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "utility", "bridge"}, {{"railway", "rail"}, {"usage", "military"}, {"service", "spur"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "spur", "bridge"}, {{"railway", "rail"}, {"service", "spur"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "service", "bridge"}, {{"railway", "rail"}, {"service", "yard"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "service", "bridge"}, {{"railway", "rail"}, {"highspeed", "positive_value"}, {"service", "siding"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "service", "bridge"}, {{"railway", "rail"}, {"usage", "main"}, {"service", "yard"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "service", "bridge"}, {{"railway", "rail"}, {"usage", "branch"}, {"service", "crossover"}, {"bridge", "positive_value"}}},
    {{"railway", "rail", "service", "bridge"}, {{"railway", "rail"}, {"usage", "unsupported_value"}, {"service", "siding"}, {"bridge", "positive_value"}}},
    // TODO: better match to railway-rail-spur-bridge:
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "unsupported_value"}, {"bridge", "positive_value"}}},
    // TODO: better match following 3 cases to railway-rail-service-bridge:
    {{"railway", "rail"}, {{"railway", "rail"}, {"service", "unsupported_value"}, {"bridge", "positive_value"}}},
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "main"}, {"service", "unsupported_value"}, {"bridge", "positive_value"}}},
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "unsupported_value"}, {"service", "unsupported_value"}, {"bridge", "positive_value"}}},

    // Tunnels (note, railway-rail-tunnel should be never matched).
    {{"railway", "rail", "highspeed", "tunnel"}, {{"railway", "rail"}, {"highspeed", "positive_value"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "highspeed", "tunnel"}, {{"railway", "rail"}, {"usage", "main"}, {"highspeed", "positive_value"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "tourism", "tunnel"}, {{"railway", "rail"}, {"usage", "tourism"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "main", "tunnel"}, {{"railway", "rail"}, {"usage", "main"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "branch", "tunnel"}, {{"railway", "rail"}, {"usage", "branch"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "branch", "tunnel"}, {{"railway", "rail"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "utility", "tunnel"}, {{"railway", "rail"}, {"usage", "industrial"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "utility", "tunnel"}, {{"railway", "rail"}, {"usage", "military"}, {"service", "spur"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "spur", "tunnel"}, {{"railway", "rail"}, {"service", "spur"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "service", "tunnel"}, {{"railway", "rail"}, {"service", "yard"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "service", "tunnel"}, {{"railway", "rail"}, {"highspeed", "positive_value"}, {"service", "siding"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "service", "tunnel"}, {{"railway", "rail"}, {"usage", "main"}, {"service", "yard"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "service", "tunnel"}, {{"railway", "rail"}, {"usage", "branch"}, {"service", "crossover"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail", "service", "tunnel"}, {{"railway", "rail"}, {"usage", "unsupported_value"}, {"service", "siding"}, {"tunnel", "positive_value"}}},
    // TODO: better match to railway-rail-spur-tunnel:
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "unsupported_value"}, {"tunnel", "positive_value"}}},
    // TODO: better match following 3 cases to railway-rail-service-tunnel:
    {{"railway", "rail"}, {{"railway", "rail"}, {"service", "unsupported_value"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "main"}, {"service", "unsupported_value"}, {"tunnel", "positive_value"}}},
    {{"railway", "rail"}, {{"railway", "rail"}, {"usage", "unsupported_value"}, {"service", "unsupported_value"}, {"tunnel", "positive_value"}}},
  };

  for (auto const & type : railTypes)
  {
    auto const params = GetFeatureBuilderParams(type.second);
    TEST_EQUAL(params.m_types.size(), 1, (type, params));
    TEST(params.IsTypeExist(GetType(type.first)), (type, params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_SimpleTypesSmoke)
{
  Tags const oneTypes = {
    // Filtered out by MatchTypes filter because have no styles.
    // {"aeroway", "apron"},
    // {"area:highway", "cycleway"},
    // {"area:highway", "motorway"},
    // {"area:highway", "path"},
    // {"area:highway", "primary"},
    // {"area:highway", "residential"},
    // {"area:highway", "secondary"},
    // {"area:highway", "service"},
    // {"area:highway", "steps"},
    // {"area:highway", "tertiary"},
    // {"area:highway", "track"},
    // {"area:highway", "trunk"},
    // {"area:highway", "unclassified"},
    {"barrier", "cycle_barrier"},
    // {"boundary", "administrative"},
    // {"communication", "line"},
    // {"earthquake:damage", "spontaneous_camp"},
    // {"highway", "traffic_signals"},
    // {"historic", "battlefield"},
    // {"landuse", "brownfield"},
    // {"landuse", "farmyard"},
    // {"landuse", "greenfield"},
    // {"landuse", "greenhouse_horticulture"},
    // {"landuse", "retail"},
    {"man_made", "cairn"},
    // {"man_made", "pipeline"},
    // {"man_made", "surveillance"},
    {"man_made", "tower"},
    // {"man_made", "wastewater_plant"},
    {"man_made", "water_tower"},
    {"man_made", "water_well"},
    // {"mapswithme", "grid"},
    // {"military", "bunker"},
    // {"natural", "cliff"},
    // {"natural", "meadow"},
    // {"natural", "orchard"},
    // {"natural", "tree"},
    // {"natural", "tree_row"},
    // {"natural", "vineyard"},
    // {"noexit"},
    // {"place", "county"},
    // {"power", "generator"},
    // {"power", "minor_line"},
    // {"power", "pole"},
    // {"railway", "crossing"},
    // {"railway", "razed"},
    // {"railway", "siding"},
    // {"railway", "spur"},
    // {"railway", "subway"},
    // {"traffic_calming", "bump"},
    // {"traffic_calming", "hump"},
    {"addr:interpolation", "even"},
    {"addr:interpolation", "odd"},
    {"aerialway", "cable_car"},
    {"aerialway", "chair_lift"},
    {"aerialway", "drag_lift"},
    {"aerialway", "gondola"},
    {"aerialway", "mixed_lift"},
    {"aerialway", "station"},
    {"aeroway", "aerodrome"},
    {"aeroway", "gate"},
    {"aeroway", "helipad"},
    {"aeroway", "runway"},
    {"aeroway", "taxiway"},
    {"aeroway", "terminal"},
    {"amenity", "arts_centre"},
    {"amenity", "atm"},
    {"amenity", "bank"},
    {"amenity", "bar"},
    {"amenity", "bbq"},
    {"amenity", "bench"},
    {"amenity", "bicycle_parking"},
    {"amenity", "bicycle_rental"},
    {"amenity", "biergarten"},
    {"amenity", "brothel"},
    {"amenity", "bureau_de_change"},
    {"amenity", "bus_station"},
    {"amenity", "cafe"},
    {"amenity", "car_rental"},
    {"amenity", "car_sharing"},
    {"amenity", "car_wash"},
    {"amenity", "casino"},
    {"amenity", "charging_station"},
    {"amenity", "childcare"},
    {"amenity", "cinema"},
    {"amenity", "clinic"},
    {"amenity", "college"},
    {"amenity", "community_centre"},
    {"amenity", "courthouse"},
    {"amenity", "dentist"},
    {"amenity", "doctors"},
    {"amenity", "drinking_water"},
    {"amenity", "driving_school"},
    {"amenity", "fast_food"},
    {"amenity", "ferry_terminal"},
    {"amenity", "fire_station"},
    {"amenity", "food_court"},
    {"amenity", "fountain"},
    {"amenity", "fuel"},
    {"amenity", "grave_yard"},
    {"amenity", "hospital"},
    {"amenity", "hunting_stand"},
    {"amenity", "ice_cream"},
    {"amenity", "internet_cafe"},
    {"amenity", "kindergarten"},
    {"amenity", "library"},
    {"amenity", "marketplace"},
    {"amenity", "motorcycle_parking"},
    {"amenity", "nightclub"},
    {"amenity", "nursing_home"},
    {"amenity", "parking"},
    {"amenity", "parking_space"},
    {"amenity", "payment_terminal"},
    {"amenity", "pharmacy"},
    {"amenity", "place_of_worship"},
    {"amenity", "police"},
    {"amenity", "post_box"},
    {"amenity", "post_office"},
    {"amenity", "prison"},
    {"amenity", "pub"},
    {"amenity", "public_bookcase"},
    {"amenity", "restaurant"},
    {"amenity", "school"},
    {"amenity", "shelter"},
    {"amenity", "shower"},
    {"amenity", "taxi"},
    {"amenity", "telephone"},
    {"amenity", "theatre"},
    {"amenity", "toilets"},
    {"amenity", "townhall"},
    {"amenity", "university"},
    {"amenity", "veterinary"},
    {"amenity", "waste_basket"},
    {"amenity", "waste_disposal"},
    {"amenity", "water_point"},
    {"area:highway", "footway"},
    {"area:highway", "living_street"},
    {"area:highway", "pedestrian"},
    {"barrier", "block"},
    {"barrier", "bollard"},
    {"barrier", "border_control"},
    {"barrier", "chain"},
    {"barrier", "city_wall"},
    {"barrier", "entrance"},
    {"barrier", "fence"},
    {"barrier", "gate"},
    {"barrier", "hedge"},
    {"barrier", "lift_gate"},
    {"barrier", "retaining_wall"},
    {"barrier", "stile"},
    {"barrier", "swing_gate"},
    {"barrier", "toll_booth"},
    {"barrier", "wall"},
    {"boundary", "national_park"},
    {"boundary", "protected_area"},
    {"building", "has_parts"},
    {"building", "train_station"},
    {"cemetery", "grave"},
    {"craft", "beekeeper"},
    {"craft", "blacksmith"},
    {"craft", "brewery"},
    {"craft", "carpenter"},
    {"craft", "confectionery"},
    {"craft", "electrician"},
    {"craft", "electronics_repair"},
    {"craft", "gardener"},
    {"craft", "handicraft"},
    {"craft", "hvac"},
    {"craft", "metal_construction"},
    {"craft", "painter"},
    {"craft", "photographer"},
    {"craft", "plumber"},
    {"craft", "sawmill"},
    {"craft", "shoemaker"},
    {"craft", "tailor"},
    {"craft", "winery"},
    {"cuisine", "african"},
    {"cuisine", "american"},
    {"cuisine", "arab"},
    {"cuisine", "argentinian"},
    {"cuisine", "asian"},
    {"cuisine", "austrian"},
    {"cuisine", "bagel"},
    {"cuisine", "balkan"},
    {"cuisine", "barbecue"},
    {"cuisine", "bavarian"},
    {"cuisine", "beef_bowl"},
    {"cuisine", "brazilian"},
    {"cuisine", "breakfast"},
    {"cuisine", "burger"},
    {"cuisine", "buschenschank"},
    {"cuisine", "cake"},
    {"cuisine", "caribbean"},
    {"cuisine", "chicken"},
    {"cuisine", "chinese"},
    {"cuisine", "coffee_shop"},
    {"cuisine", "crepe"},
    {"cuisine", "croatian"},
    {"cuisine", "curry"},
    {"cuisine", "deli"},
    {"cuisine", "diner"},
    {"cuisine", "donut"},
    {"cuisine", "ethiopian"},
    {"cuisine", "filipino"},
    {"cuisine", "fine_dining"},
    {"cuisine", "fish"},
    {"cuisine", "fish_and_chips"},
    {"cuisine", "french"},
    {"cuisine", "friture"},
    {"cuisine", "georgian"},
    {"cuisine", "german"},
    {"cuisine", "greek"},
    {"cuisine", "grill"},
    {"cuisine", "heuriger"},
    {"cuisine", "hotdog"},
    {"cuisine", "hungarian"},
    {"cuisine", "ice_cream"},
    {"cuisine", "indian"},
    {"cuisine", "indonesian"},
    {"cuisine", "international"},
    {"cuisine", "irish"},
    {"cuisine", "italian"},
    {"cuisine", "italian_pizza"},
    {"cuisine", "japanese"},
    {"cuisine", "kebab"},
    {"cuisine", "korean"},
    {"cuisine", "lao"},
    {"cuisine", "lebanese"},
    {"cuisine", "local"},
    {"cuisine", "malagasy"},
    {"cuisine", "malaysian"},
    {"cuisine", "mediterranean"},
    {"cuisine", "mexican"},
    {"cuisine", "moroccan"},
    {"cuisine", "noodles"},
    {"cuisine", "oriental"},
    {"cuisine", "pancake"},
    {"cuisine", "pasta"},
    {"cuisine", "persian"},
    {"cuisine", "peruvian"},
    {"cuisine", "pizza"},
    {"cuisine", "polish"},
    {"cuisine", "portuguese"},
    {"cuisine", "ramen"},
    {"cuisine", "regional"},
    {"cuisine", "russian"},
    {"cuisine", "sandwich"},
    {"cuisine", "sausage"},
    {"cuisine", "savory_pancakes"},
    {"cuisine", "seafood"},
    {"cuisine", "soba"},
    {"cuisine", "spanish"},
    {"cuisine", "steak_house"},
    {"cuisine", "sushi"},
    {"cuisine", "tapas"},
    {"cuisine", "tea"},
    {"cuisine", "thai"},
    {"cuisine", "turkish"},
    {"cuisine", "vegan"},
    {"cuisine", "vegetarian"},
    {"cuisine", "vietnamese"},
    {"emergency", "defibrillator"},
    {"emergency", "fire_hydrant"},
    {"emergency", "phone"},
    {"fee", "no"},
    {"fee", "yes"},
    {"highway", "bridleway"},
    {"highway", "busway"},
    {"highway", "bus_stop"},
    {"highway", "construction"},
    {"highway", "cycleway"},
    {"highway", "footway"},
    {"highway", "ford"},
    {"highway", "living_street"},
    {"highway", "motorway"},
    {"highway", "motorway_junction"},
    {"highway", "motorway_link"},
    {"highway", "path"},
    {"highway", "pedestrian"},
    {"highway", "primary"},
    {"highway", "primary_link"},
    {"highway", "raceway"},
    {"highway", "residential"},
    {"highway", "rest_area"},
    {"highway", "road"},
    {"highway", "secondary"},
    {"highway", "secondary_link"},
    {"highway", "service"},
    {"highway", "services"},
    {"highway", "speed_camera"},
    {"highway", "steps"},
    {"highway", "tertiary"},
    {"highway", "tertiary_link"},
    {"highway", "track"},
    {"highway", "trunk"},
    {"highway", "trunk_link"},
    {"highway", "unclassified"},
    {"highway", "world_level"},
    {"highway", "world_towns_level"},
    {"historic", "archaeological_site"},
    {"historic", "boundary_stone"},
    {"historic", "castle"},
    {"historic", "city_gate"},
    {"historic", "citywalls"},
    {"historic", "fort"},
    {"historic", "memorial"},
    {"historic", "monument"},
    {"historic", "ruins"},
    {"historic", "ship"},
    {"historic", "tomb"},
    {"historic", "wayside_cross"},
    {"historic", "wayside_shrine"},
    {"hwtag", "bidir_bicycle"},
    {"hwtag", "lit"},
    {"hwtag", "nobicycle"},
    {"hwtag", "nocar"},
    {"hwtag", "nofoot"},
    {"hwtag", "oneway"},
    {"hwtag", "private"},
    {"hwtag", "toll"},
    {"hwtag", "yesbicycle"},
    {"hwtag", "yescar"},
    {"hwtag", "yesfoot"},
    {"internet_access", "wlan"},
    {"junction", "circular"},
    {"junction", "roundabout"},
    {"landuse", "allotments"},
    {"landuse", "basin"},
    {"landuse", "cemetery"},
    {"landuse", "churchyard"},
    {"landuse", "commercial"},
    {"landuse", "construction"},
    {"landuse", "farmland"},
    {"landuse", "field"},
    {"landuse", "garages"},
    {"landuse", "grass"},
    {"landuse", "industrial"},
    {"landuse", "landfill"},
    {"landuse", "meadow"},
    {"landuse", "military"},
    {"landuse", "orchard"},
    {"landuse", "quarry"},
    {"landuse", "railway"},
    {"landuse", "recreation_ground"},
    {"landuse", "reservoir"},
    {"landuse", "residential"},
    {"landuse", "salt_pond"},
    {"landuse", "village_green"},
    {"landuse", "vineyard"},
    {"leisure", "common"},
    {"leisure", "dog_park"},
    {"leisure", "fitness_centre"},
    {"leisure", "fitness_station"},
    {"leisure", "garden"},
    {"leisure", "golf_course"},
    {"leisure", "ice_rink"},
    {"leisure", "marina"},
    {"leisure", "nature_reserve"},
    {"leisure", "park"},
    {"leisure", "pitch"},
    {"leisure", "playground"},
    {"leisure", "recreation_ground"},
    {"leisure", "resort"},
    {"leisure", "sauna"},
    {"leisure", "slipway"},
    {"leisure", "sports_centre"},
    {"leisure", "stadium"},
    {"leisure", "swimming_pool"},
    {"leisure", "track"},
    {"leisure", "water_park"},
    {"man_made", "breakwater"},
    {"man_made", "chimney"},
    {"man_made", "cutline"},
    {"man_made", "embankment"},
    {"man_made", "lighthouse"},
    {"man_made", "survey_point"},
    {"man_made", "pier"},
    {"man_made", "silo"},
    {"man_made", "storage_tank"},
    {"man_made", "water_tap"},
    {"man_made", "windmill"},
    {"man_made", "works"},
    {"natural", "bare_rock"},
    {"natural", "bay"},
    {"natural", "beach"},
    {"natural", "cape"},
    {"natural", "cave_entrance"},
    {"natural", "cliff"},
    {"natural", "coastline"},
    {"natural", "desert"},
    {"natural", "earth_bank"},
    {"natural", "geyser"},
    {"natural", "glacier"},
    {"natural", "grassland"},
    {"natural", "heath"},
    {"natural", "hot_spring"},
    {"natural", "land"},
    {"natural", "peak"},
    {"natural", "rock"},
    {"natural", "saddle"},
    {"natural", "scrub"},
    {"natural", "spring"},
    {"natural", "volcano"},
    {"natural", "water"},
    {"natural", "wetland"},
    {"office", "company"},
    {"office", "diplomatic"},
    {"office", "estate_agent"},
    {"office", "government"},
    {"office", "insurance"},
    {"office", "lawyer"},
    {"office", "ngo"},
    {"office", "telecommunication"},
    {"organic", "only"},
    {"organic", "yes"},
    {"aerialway", "j-bar"},
    {"aerialway", "magic_carpet"},
    {"aerialway", "platter"},
    {"aerialway", "rope_tow"},
    {"aerialway", "t-bar"},
    {"piste:type", "downhill"},
    {"piste:type", "nordic"},
    {"piste:type", "sled"},
    {"place", "city"},
    {"place", "continent"},
    {"place", "country"},
    {"place", "farm"},
    {"place", "hamlet"},
    {"place", "island"},
    {"place", "islet"},
    {"place", "isolated_dwelling"},
    {"place", "locality"},
    {"place", "neighbourhood"},
    {"place", "ocean"},
    {"place", "region"},
    {"place", "sea"},
    {"place", "square"},
    {"place", "state"},
    {"place", "suburb"},
    {"place", "town"},
    {"place", "village"},
    {"power", "line"},
    {"power", "station"},
    {"power", "substation"},
    {"power", "tower"},
    {"psurface", "paved_bad"},
    {"psurface", "paved_good"},
    {"psurface", "unpaved_bad"},
    {"psurface", "unpaved_good"},
    {"public_transport", "platform"},
    {"railway", "abandoned"},
    {"railway", "construction"},
    {"railway", "disused"},
    {"railway", "funicular"},
    {"railway", "halt"},
    {"railway", "level_crossing"},
    {"railway", "light_rail"},
    {"railway", "monorail"},
    {"railway", "narrow_gauge"},
    {"railway", "platform"},
    {"railway", "preserved"},
    {"railway", "station"},
    {"railway", "subway_entrance"},
    {"railway", "tram"},
    {"railway", "tram_stop"},
    {"shop", "alcohol"},
    {"shop", "bakery"},
    {"shop", "beauty"},
    {"shop", "beverages"},
    {"shop", "bicycle"},
    {"shop", "bookmaker"},
    {"shop", "books"},
    {"shop", "butcher"},
    {"shop", "car"},
    {"shop", "car_parts"},
    {"shop", "car_repair"},
    {"shop", "chemist"},
    {"shop", "chocolate"},
    {"shop", "coffee"},
    {"shop", "computer"},
    {"shop", "confectionery"},
    {"shop", "convenience"},
    {"shop", "copyshop"},
    {"shop", "cosmetics"},
    {"shop", "deli"},
    {"shop", "department_store"},
    {"shop", "doityourself"},
    {"shop", "dry_cleaning"},
    {"shop", "electronics"},
    {"shop", "erotic"},
    {"shop", "fabric"},
    {"shop", "farm"},
    {"shop", "florist"},
    {"shop", "funeral_directors"},
    {"shop", "furniture"},
    {"shop", "garden_centre"},
    {"shop", "gift"},
    {"shop", "greengrocer"},
    {"shop", "grocery"},
    {"shop", "hairdresser"},
    {"shop", "hardware"},
    {"shop", "houseware"},
    {"shop", "health_food"},
    {"shop", "jewelry"},
    {"shop", "kiosk"},
    {"shop", "kitchen"},
    {"shop", "laundry"},
    {"shop", "mall"},
    {"shop", "massage"},
    {"shop", "mobile_phone"},
    {"shop", "money_lender"},
    {"shop", "motorcycle"},
    {"shop", "music"},
    {"shop", "musical_instrument"},
    {"shop", "newsagent"},
    {"shop", "optician"},
    {"shop", "outdoor"},
    {"shop", "pastry"},
    {"shop", "pawnbroker"},
    {"shop", "pet"},
    {"shop", "photo"},
    {"shop", "seafood"},
    {"shop", "second_hand"},
    {"shop", "sewing"},
    {"shop", "shoes"},
    {"shop", "sports"},
    {"shop", "stationery"},
    {"shop", "supermarket"},
    {"shop", "tattoo"},
    {"shop", "tea"},
    {"shop", "ticket"},
    {"shop", "toys"},
    {"shop", "travel_agency"},
    {"shop", "tyres"},
    {"shop", "variety_store"},
    {"shop", "video"},
    {"shop", "wine"},
    {"sport", "american_football"},
    {"sport", "archery"},
    {"sport", "athletics"},
    {"sport", "australian_football"},
    {"sport", "baseball"},
    {"sport", "basketball"},
    {"sport", "bowls"},
    {"sport", "cricket"},
    {"sport", "curling"},
    {"sport", "diving"},
    {"sport", "equestrian"},
    {"sport", "gymnastics"},
    {"sport", "handball"},
    {"sport", "multi"},
    {"sport", "scuba_diving"},
    {"sport", "shooting"},
    {"sport", "skiing"},
    {"sport", "soccer"},
    {"sport", "swimming"},
    {"sport", "tennis"},
    {"tourism", "alpine_hut"},
    {"tourism", "apartment"},
    {"tourism", "artwork"},
    {"tourism", "attraction"},
    {"tourism", "camp_site"},
    {"tourism", "caravan_site"},
    {"tourism", "chalet"},
    {"tourism", "gallery"},
    {"tourism", "guest_house"},
    {"tourism", "hostel"},
    {"tourism", "hotel"},
    {"tourism", "information"},
    {"tourism", "motel"},
    {"tourism", "museum"},
    {"tourism", "picnic_site"},
    {"tourism", "theme_park"},
    {"tourism", "viewpoint"},
    {"tourism", "wilderness_hut"},
    {"tourism", "zoo"},
    {"waterway", "canal"},
    {"waterway", "dam"},
    {"waterway", "ditch"},
    {"waterway", "dock"},
    {"waterway", "drain"},
    {"waterway", "lock_gate"},
    {"waterway", "river"},
    {"waterway", "stream"},
    {"waterway", "waterfall"},
    {"waterway", "weir"},
    {"wheelchair", "limited"},
    {"wheelchair", "no"},
    {"wheelchair", "yes"},
  };

  using SV = std::string_view;

  auto const & cl = classif();
  for (auto const & type : oneTypes)
  {
    auto const params = GetFeatureBuilderParams({type});
    TEST_EQUAL(params.m_types.size(), 1, (type, params));
    TEST(params.IsTypeExist(cl.GetTypeByPath({SV(type.m_key), SV(type.m_value)})), (type, params));
  }

  Tags const exTypes = {
      {"route", "ferry"},
      {"route", "shuttle_train"},
  };

  for (auto const & type : exTypes)
  {
    auto const params = GetFeatureBuilderParams({type});
    TEST_GREATER(params.m_types.size(), 1, (type, params));
    TEST(params.IsTypeExist(cl.GetTypeByPath({SV(type.m_key), SV(type.m_value)})), (type, params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_ComplexTypesSmoke)
{
  using Type = std::vector<std::string>;
  std::vector<std::pair<Type, Tags>> const complexTypes = {
    // Filtered out by MatchTypes filter because have no styles.
    // {{"communication", "line", "underground"}, {{"communication", "line"}, {"location", "underground"}}},
    // {{"man_made", "pipeline", "overground"}, {{"man_made", "pipeline"}, {"location", "overground"}}},
    // {{"railway", "siding", "bridge"}, {{"railway", "siding"}, {"bridge", "any_value"}}},
    // {{"railway", "siding", "tunnel"}, {{"railway", "siding"}, {"tunnel", "any_value"}}},
    // {{"railway", "spur", "bridge"}, {{"railway", "spur"}, {"bridge", "any_value"}}},
    // {{"railway", "spur", "tunnel"}, {{"railway", "spur"}, {"tunnel", "any_value"}}},
    // {{"waterway", "ditch", "tunnel"}, {{"waterway", "ditch"}, {"tunnel", "any_value"}}},
    // {{"waterway", "drain", "tunnel"}, {{"waterway", "drain"}, {"tunnel", "any_value"}}},
    // {{"waterway", "stream", "tunnel"}, {{"waterway", "stream"}, {"tunnel", "any_value"}}},
    //
    // two types (+hwtag yesbicycle) {{"highway", "path", "bicycle"}, {{"highway", "path"}, {"bicycle", "any_value"}}},
    // two types (+hwtag-private) {{"highway", "track", "no-access"}, {{"highway", "track"}, {"access", "no"}}},
    // two types (+office) {{"tourism", "information", "office"}, {{"tourism", "information"}, {"office", "any_value"}}},
    // two types (+sport-*) {{"leisure", "sports_centre"}, {{"leisure", "sports_centre"}, {"sport", "any_value"}}},
    //
    // Manually constructed type, not parsed from osm.
    // {{"building", "address"}, {{"addr:housenumber", "any_value"}, {"addr:street", "any_value"}}},
    {{"addr:interpolation"}, {{"addr:interpolation", "all"}}},
    {{"aeroway", "aerodrome", "international"}, {{"aeroway", "aerodrome"}, {"aerodrome", "international"}}},
    {{"amenity", "grave_yard", "christian"}, {{"amenity", "grave_yard"}, {"religion", "christian"}}},
    {{"amenity", "parking", "lane"}, {{"amenity", "parking"}, {"parking", "lane"}}},
    {{"amenity", "parking", "multi-storey"}, {{"amenity", "parking"}, {"parking", "multi-storey"}}},
    {{"amenity", "parking", "no-access"}, {{"amenity", "parking"}, {"access", "no"}}},
    {{"amenity", "parking", "park_and_ride"}, {{"amenity", "parking"}, {"parking", "park_and_ride"}}},
    {{"amenity", "parking", "permissive"}, {{"amenity", "parking"}, {"access", "permissive"}}},
    {{"amenity", "parking", "private"}, {{"amenity", "parking"}, {"access", "private"}}},
    {{"amenity", "parking", "street_side"}, {{"amenity", "parking"}, {"parking", "street_side"}}},
    {{"amenity", "parking", "underground"}, {{"amenity", "parking"}, {"location", "underground"}}},
    {{"amenity", "parking_space", "permissive"}, {{"amenity", "parking_space"}, {"access", "permissive"}}},
    {{"amenity", "parking_space", "private"}, {{"amenity", "parking_space"}, {"access", "private"}}},
    {{"amenity", "parking_space", "underground"}, {{"amenity", "parking_space"}, {"parking", "underground"}}},
    {{"amenity", "place_of_worship", "buddhist"}, {{"amenity", "place_of_worship"}, {"religion", "buddhist"}}},
    {{"amenity", "place_of_worship", "christian"}, {{"amenity", "place_of_worship"}, {"religion", "christian"}}},
    {{"amenity", "place_of_worship", "hindu"}, {{"amenity", "place_of_worship"}, {"religion", "hindu"}}},
    {{"amenity", "place_of_worship", "jewish"}, {{"amenity", "place_of_worship"}, {"religion", "jewish"}}},
    {{"amenity", "place_of_worship", "muslim"}, {{"amenity", "place_of_worship"}, {"religion", "muslim"}}},
    {{"amenity", "place_of_worship", "shinto"}, {{"amenity", "place_of_worship"}, {"religion", "shinto"}}},
    {{"amenity", "place_of_worship", "taoist"}, {{"amenity", "place_of_worship"}, {"religion", "taoist"}}},
    {{"amenity", "recycling", "centre"}, {{"amenity", "recycling"}, {"recycling_type","centre"}}},
    {{"amenity", "recycling", "container"}, {{"amenity", "recycling"}, {"recycling_type","container"}}},
    {{"amenity", "recycling"}, {{"amenity", "recycling"}}},
    {{"amenity", "parcel_locker"}, {{"amenity", "parcel_locker"}}},
    {{"amenity", "vending_machine", "cigarettes"}, {{"amenity", "vending_machine"}, {"vending", "cigarettes"}}},
    {{"amenity", "vending_machine", "coffee"}, {{"amenity", "vending_machine"}, {"vending", "coffee"}}},
    {{"amenity", "vending_machine", "condoms"}, {{"amenity", "vending_machine"}, {"vending", "condoms"}}},
    {{"amenity", "vending_machine", "drinks"}, {{"amenity", "vending_machine"}, {"vending", "drinks"}}},
    {{"amenity", "vending_machine", "food"}, {{"amenity", "vending_machine"}, {"vending", "food"}}},
    {{"amenity", "vending_machine", "parking_tickets"}, {{"amenity", "vending_machine"}, {"vending", "parking_tickets"}}},
    {{"amenity", "vending_machine", "public_transport_tickets"}, {{"amenity", "vending_machine"}, {"vending", "public_transport_tickets"}}},
    {{"amenity", "vending_machine", "newspapers"}, {{"amenity", "vending_machine"}, {"vending", "newspapers"}}},
    {{"amenity", "vending_machine", "sweets"}, {{"amenity", "vending_machine"}, {"vending", "sweets"}}},
    {{"amenity"}, {{"amenity", "any_value"}}},
    {{"boundary", "administrative", "2"}, {{"boundary", "administrative"}, {"admin_level", "2"}}},
    {{"boundary", "administrative", "3"}, {{"boundary", "administrative"}, {"admin_level", "3"}}},
    {{"boundary", "administrative", "4"}, {{"boundary", "administrative"}, {"admin_level", "4"}}},
    {{"building", "garage"}, {{"building", "garage"}}},
    {{"building", "garage"}, {{"building", "yes"}, {"garage", "any_value"}}},
    {{"building"}, {{"building", "any_value"}}},
    {{"building:part"}, {{"building:part", "any_value"}}},
    {{"entrance"}, {{"entrance", "any_value"}}},
    {{"highway", "bridleway", "bridge"}, {{"highway", "bridleway"}, {"bridge", "any_value"}}},
    {{"highway", "bridleway", "tunnel"}, {{"highway", "bridleway"}, {"tunnel", "any_value"}}},
    {{"highway", "busway"}, {{"highway", "service"}, {"service", "bus"}}},
    {{"highway", "busway"}, {{"highway", "service"}, {"service", "busway"}}},
    {{"highway", "busway", "bridge"}, {{"highway", "busway"}, {"bridge", "any_value"}}},
    {{"highway", "busway", "tunnel"}, {{"highway", "busway"}, {"tunnel", "any_value"}}},
    {{"highway", "cycleway", "bridge"}, {{"highway", "cycleway"}, {"bridge", "any_value"}}},
    {{"highway", "cycleway", "tunnel"}, {{"highway", "cycleway"}, {"tunnel", "any_value"}}},
    {{"highway", "footway", "area"}, {{"highway", "footway"}, {"area", "any_value"}}},
    {{"highway", "footway", "bridge"}, {{"highway", "footway"}, {"bridge", "any_value"}}},
    {{"highway", "footway"}, {{"highway", "footway"}, {"footway", "unsupported_value"}}},
    {{"highway", "footway", "sidewalk"}, {{"highway", "footway"}, {"footway", "sidewalk"}}},
    {{"highway", "footway", "crossing"}, {{"highway", "footway"}, {"footway", "crossing"}}},
    {{"highway", "footway", "tunnel"}, {{"highway", "footway"}, {"tunnel", "any_value"}}},
    {{"highway", "footway", "tunnel"}, {{"highway", "footway"}, {"location", "underground"}}},
    {{"highway", "living_street", "bridge"}, {{"highway", "living_street"}, {"bridge", "any_value"}}},
    {{"highway", "living_street", "tunnel"}, {{"highway", "living_street"}, {"tunnel", "any_value"}}},
    {{"highway", "motorway", "bridge"}, {{"highway", "motorway"}, {"bridge", "any_value"}}},
    {{"highway", "motorway", "tunnel"}, {{"highway", "motorway"}, {"tunnel", "any_value"}}},
    {{"highway", "motorway_link", "bridge"}, {{"highway", "motorway_link"}, {"bridge", "any_value"}}},
    {{"highway", "motorway_link", "tunnel"}, {{"highway", "motorway_link"}, {"tunnel", "any_value"}}},
    {{"highway", "path", "bridge"}, {{"highway", "path"}, {"bridge", "any_value"}}},
    {{"highway", "path", "horse"}, {{"highway", "path"}, {"horse", "any_value"}}},
    {{"highway", "path", "tunnel"}, {{"highway", "path"}, {"tunnel", "any_value"}}},
    {{"highway", "path", "difficult"}, {{"highway", "path"}, {"_path_grade", "difficult"}}},
    {{"highway", "path", "expert"}, {{"highway", "path"}, {"_path_grade", "expert"}}},
    {{"highway", "pedestrian", "area"}, {{"highway", "pedestrian"}, {"area", "any_value"}}},
    {{"highway", "pedestrian", "bridge"}, {{"highway", "pedestrian"}, {"bridge", "any_value"}}},
    {{"highway", "pedestrian", "tunnel"}, {{"highway", "pedestrian"}, {"tunnel", "any_value"}}},
    {{"highway", "primary", "bridge"}, {{"highway", "primary"}, {"bridge", "any_value"}}},
    {{"highway", "primary", "tunnel"}, {{"highway", "primary"}, {"tunnel", "any_value"}}},
    {{"highway", "primary_link", "bridge"}, {{"highway", "primary_link"}, {"bridge", "any_value"}}},
    {{"highway", "primary_link", "tunnel"}, {{"highway", "primary_link"}, {"tunnel", "any_value"}}},
    {{"highway", "residential", "area"}, {{"highway", "residential"}, {"area", "any_value"}}},
    {{"highway", "residential", "bridge"}, {{"highway", "residential"}, {"bridge", "any_value"}}},
    {{"highway", "residential", "tunnel"}, {{"highway", "residential"}, {"tunnel", "any_value"}}},
    {{"highway", "road", "bridge"}, {{"highway", "road"}, {"bridge", "any_value"}}},
    {{"highway", "road", "tunnel"}, {{"highway", "road"}, {"tunnel", "any_value"}}},
    {{"highway", "secondary", "bridge"}, {{"highway", "secondary"}, {"bridge", "any_value"}}},
    {{"highway", "secondary", "tunnel"}, {{"highway", "secondary"}, {"tunnel", "any_value"}}},
    {{"highway", "secondary_link", "bridge"}, {{"highway", "secondary_link"}, {"bridge", "any_value"}}},
    {{"highway", "secondary_link", "tunnel"}, {{"highway", "secondary_link"}, {"tunnel", "any_value"}}},
    {{"highway", "service", "area"}, {{"highway", "service"}, {"area", "any_value"}}},
    {{"highway", "service", "bridge"}, {{"highway", "service"}, {"bridge", "any_value"}}},
    {{"highway", "service", "driveway"}, {{"highway", "service"}, {"service", "driveway"}}},
    {{"highway", "service", "parking_aisle"}, {{"highway", "service"}, {"service", "parking_aisle"}}},
    {{"highway", "service", "tunnel"}, {{"highway", "service"}, {"tunnel", "any_value"}}},
    {{"highway", "steps", "bridge"}, {{"highway", "steps"}, {"bridge", "any_value"}}},
    {{"highway", "steps", "tunnel"}, {{"highway", "steps"}, {"tunnel", "any_value"}}},
    {{"highway", "tertiary", "bridge"}, {{"highway", "tertiary"}, {"bridge", "any_value"}}},
    {{"highway", "tertiary", "tunnel"}, {{"highway", "tertiary"}, {"tunnel", "any_value"}}},
    {{"highway", "tertiary_link", "bridge"}, {{"highway", "tertiary_link"}, {"bridge", "any_value"}}},
    {{"highway", "tertiary_link", "tunnel"}, {{"highway", "tertiary_link"}, {"tunnel", "any_value"}}},
    {{"highway", "track", "area"}, {{"highway", "track"}, {"area", "any_value"}}},
    {{"highway", "track", "bridge"}, {{"highway", "track"}, {"bridge", "any_value"}}},
    {{"highway", "track", "tunnel"}, {{"highway", "track"}, {"tunnel", "any_value"}}},
    {{"highway", "trunk", "bridge"}, {{"highway", "trunk"}, {"bridge", "any_value"}}},
    {{"highway", "trunk", "tunnel"}, {{"highway", "trunk"}, {"tunnel", "any_value"}}},
    {{"highway", "trunk_link", "bridge"}, {{"highway", "trunk_link"}, {"bridge", "any_value"}}},
    {{"highway", "trunk_link", "tunnel"}, {{"highway", "trunk_link"}, {"tunnel", "any_value"}}},
    {{"highway", "unclassified", "area"}, {{"highway", "unclassified"}, {"area", "any_value"}}},
    {{"highway", "unclassified", "bridge"}, {{"highway", "unclassified"}, {"bridge", "any_value"}}},
    {{"highway", "unclassified", "tunnel"}, {{"highway", "unclassified"}, {"tunnel", "any_value"}}},
    {{"historic", "castle", "defensive"}, {{"historic", "castle"}, {"castle_type", "defensive"}}},
    {{"historic", "castle", "fortress"}, {{"historic", "castle"}, {"castle_type", "fortress"}}},
    {{"historic", "castle", "fortress"}, {{"historic", "fortress"}}},
    {{"historic", "castle", "manor"}, {{"historic", "castle"}, {"castle_type", "manor"}}},
    {{"historic", "castle", "manor"}, {{"historic", "manor"}}},
    {{"historic", "castle", "stately"}, {{"historic", "castle"}, {"castle_type", "stately"}}},
    {{"historic", "memorial", "cross"}, {{"historic", "memorial"}, {"memorial", "cross"}}},
    {{"historic", "memorial", "plaque"}, {{"historic", "memorial"}, {"memorial", "plaque"}}},
    {{"historic", "memorial", "plaque"}, {{"historic", "memorial"}, {"memorial:type", "plaque"}}},
    {{"historic", "memorial", "plaque"}, {{"historic", "memorial"}, {"memorial:type", "plate"}}},
    {{"historic", "memorial", "sculpture"}, {{"historic", "memorial"}, {"memorial", "sculpture"}}},
    {{"historic", "memorial", "sculpture"}, {{"historic", "memorial"}, {"memorial:type", "sculpture"}}},
    {{"historic", "memorial", "statue"}, {{"historic", "memorial"}, {"memorial", "statue"}}},
    {{"historic", "memorial", "statue"}, {{"historic", "memorial"}, {"memorial:type", "statue"}}},
    {{"historic", "memorial", "stolperstein"}, {{"historic", "memorial"}, {"memorial", "stolperstein"}}},
    {{"historic", "memorial", "stolperstein"}, {{"historic", "memorial"}, {"memorial:type", "stolperstein"}}},
    {{"historic", "memorial", "war_memorial"}, {{"historic", "memorial"}, {"memorial", "war_memorial"}}},
    {{"historic", "memorial", "war_memorial"}, {{"historic", "memorial"}, {"memorial:type", "war_memorial"}}},
    {{"internet_access"}, {{"internet_access", "any_value"}}},
    {{"landuse", "cemetery", "christian"}, {{"landuse", "cemetery"}, {"religion", "christian"}}},
    {{"landuse", "forest"}, {{"landuse", "forest"}}},
    {{"landuse", "forest"}, {{"natural", "wood"}}},
    {{"landuse", "forest", "coniferous"}, {{"landuse", "forest"}, {"leaf_type", "coniferous"}}},
    {{"landuse", "forest", "coniferous"}, {{"landuse", "forest"}, {"wood", "coniferous"}}},
    {{"landuse", "forest", "coniferous"}, {{"natural", "wood"}, {"leaf_type", "coniferous"}}},
    {{"landuse", "forest", "coniferous"}, {{"natural", "wood"}, {"wood", "coniferous"}}},
    {{"landuse", "forest", "deciduous"}, {{"landuse", "forest"}, {"leaf_cycle", "deciduous"}}},
    {{"landuse", "forest", "deciduous"}, {{"landuse", "forest"}, {"leaf_type", "deciduous"}}},
    {{"landuse", "forest", "deciduous"}, {{"landuse", "forest"}, {"wood", "deciduous"}}},
    {{"landuse", "forest", "deciduous"}, {{"natural", "wood"}, {"leaf_cycle", "deciduous"}}},
    {{"landuse", "forest", "deciduous"}, {{"natural", "wood"}, {"leaf_type", "deciduous"}}},
    {{"landuse", "forest", "deciduous"}, {{"natural", "wood"}, {"wood", "deciduous"}}},
    {{"landuse", "forest", "mixed"}, {{"landuse", "forest"}, {"leaf_cycle", "mixed"}}},
    {{"landuse", "forest", "mixed"}, {{"landuse", "forest"}, {"leaf_type", "mixed"}}},
    {{"landuse", "forest", "mixed"}, {{"landuse", "forest"}, {"wood", "mixed"}}},
    {{"landuse", "forest", "mixed"}, {{"natural", "wood"}, {"leaf_cycle", "mixed"}}},
    {{"landuse", "forest", "mixed"}, {{"natural", "wood"}, {"leaf_type", "mixed"}}},
    {{"landuse", "forest", "mixed"}, {{"natural", "wood"}, {"wood", "mixed"}}},
    {{"leisure", "park", "no-access"}, {{"leisure", "park"}, {"access", "no"}}},
    {{"leisure", "park", "private"}, {{"leisure", "park"}, {"access", "private"}}},
    {{"leisure", "park", "private"}, {{"leisure", "park"}, {"access", "private"}}},
    {{"leisure", "sports_centre"}, {{"leisure", "sports_centre"}}},
    {{"leisure", "track", "area"}, {{"leisure", "track"}, {"area", "any_value"}}},
    {{"mountain_pass"}, {{"mountain_pass", "any_value"}}},
    {{"natural", "desert"}, {{"natural", "sand"}, {"desert", "erg"}}},
    {{"natural", "water", "pond"}, {{"natural", "water"}, {"water", "pond"}}},
    {{"natural", "water", "lake"}, {{"natural", "water"}, {"water", "lake"}}},
    {{"natural", "water", "reservoir"}, {{"natural", "water"}, {"water", "reservoir"}}},
    {{"natural", "water", "river"}, {{"natural", "water"}, {"water", "river"}}},
    {{"natural", "water", "basin"}, {{"natural", "water"}, {"water", "basin"}}},
    {{"natural", "water", "lock"}, {{"natural", "water"}, {"water", "lock"}}},
    {{"natural", "wetland", "bog"}, {{"natural", "wetland"}, {"wetland", "bog"}}},
    {{"natural", "wetland", "marsh"}, {{"natural", "wetland"}, {"wetland", "marsh"}}},
    {{"office"}, {{"office", "any_value"}}},
    {{"piste:type", "downhill", "advanced"}, {{"piste:type", "downhill"}, {"piste:difficulty", "advanced"}}},
    {{"piste:type", "downhill", "easy"}, {{"piste:type", "downhill"}, {"piste:difficulty", "easy"}}},
    {{"piste:type", "downhill", "expert"}, {{"piste:type", "downhill"}, {"piste:difficulty", "expert"}}},
    {{"piste:type", "downhill", "freeride"}, {{"piste:type", "downhill"}, {"piste:difficulty", "freeride"}}},
    {{"piste:type", "downhill", "intermediate"}, {{"piste:type", "downhill"}, {"piste:difficulty", "intermediate"}}},
    {{"piste:type", "downhill", "novice"}, {{"piste:type", "downhill"}, {"piste:difficulty", "novice"}}},
    {{"place", "city", "capital", "10"}, {{"place", "city"}, {"capital", "10"}}},
    {{"place", "city", "capital", "10"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "10"}}},
    {{"place", "city", "capital", "11"}, {{"place", "city"}, {"capital", "11"}}},
    {{"place", "city", "capital", "11"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "11"}}},
    {{"place", "city", "capital", "2"}, {{"place", "city"}, {"capital", "2"}}},
    {{"place", "city", "capital", "2"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "2"}}},
    {{"place", "city", "capital", "3"}, {{"place", "city"}, {"capital", "3"}}},
    {{"place", "city", "capital", "3"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "3"}}},
    {{"place", "city", "capital", "4"}, {{"place", "city"}, {"capital", "4"}}},
    {{"place", "city", "capital", "4"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "4"}}},
    {{"place", "city", "capital", "5"}, {{"place", "city"}, {"capital", "5"}}},
    {{"place", "city", "capital", "5"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "5"}}},
    {{"place", "city", "capital", "6"}, {{"place", "city"}, {"capital", "6"}}},
    {{"place", "city", "capital", "6"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "6"}}},
    {{"place", "city", "capital", "7"}, {{"place", "city"}, {"capital", "7"}}},
    {{"place", "city", "capital", "7"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "7"}}},
    {{"place", "city", "capital", "8"}, {{"place", "city"}, {"capital", "8"}}},
    {{"place", "city", "capital", "8"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "8"}}},
    {{"place", "city", "capital", "9"}, {{"place", "city"}, {"capital", "9"}}},
    {{"place", "city", "capital", "9"}, {{"place", "city"}, {"capital", "any_value"}, {"admin_level", "9"}}},
    {{"place", "city", "capital"}, {{"place", "city"}, {"capital", "any_value"}}},
    {{"place", "state", "USA"}, {{"place", "state"}, {"addr:country", "US"}}},
    {{"place", "state", "USA"}, {{"place", "state"}, {"is_in", "USA"}}},
    {{"place", "state", "USA"}, {{"place", "state"}, {"is_in:country", "USA"}}},
    {{"place", "state", "USA"}, {{"place", "state"}, {"is_in:country_code", "us"}}},
    {{"railway", "abandoned", "bridge"}, {{"railway", "abandoned"}, {"bridge", "any_value"}}},
    {{"railway", "abandoned", "tunnel"}, {{"railway", "abandoned"}, {"tunnel", "any_value"}}},
    {{"railway", "funicular", "bridge"}, {{"railway", "funicular"}, {"bridge", "any_value"}}},
    {{"railway", "funicular", "tunnel"}, {{"railway", "funicular"}, {"tunnel", "any_value"}}},
    {{"railway", "light_rail", "bridge"}, {{"railway", "light_rail"}, {"bridge", "any_value"}}},
    //{{"railway", "light_rail", "tunnel"}, {{"railway", "light_rail"}, {"tunnel", "any_value"}}},
    {{"railway", "monorail", "bridge"}, {{"railway", "monorail"}, {"bridge", "any_value"}}},
    {{"railway", "monorail", "tunnel"}, {{"railway", "monorail"}, {"tunnel", "any_value"}}},
    {{"railway", "narrow_gauge", "bridge"}, {{"railway", "narrow_gauge"}, {"bridge", "any_value"}}},
    {{"railway", "narrow_gauge", "tunnel"}, {{"railway", "narrow_gauge"}, {"tunnel", "any_value"}}},
    {{"railway", "preserved", "bridge"}, {{"railway", "preserved"}, {"bridge", "any_value"}}},
    {{"railway", "preserved", "tunnel"}, {{"railway", "preserved"}, {"tunnel", "any_value"}}},
    {{"railway", "station", "light_rail"}, {{"railway", "station"}, {"station", "light_rail"}}},
    {{"railway", "station", "light_rail"}, {{"railway", "station"}, {"transport", "light_rail"}}},
    {{"railway", "station", "monorail"}, {{"railway", "station"}, {"station", "monorail"}}},
    {{"railway", "station", "monorail"}, {{"railway", "station"}, {"transport", "monorail"}}},
    {{"railway", "station", "subway", "barcelona"}, {{"railway", "station"}, {"station", "subway"}, {"city", "barcelona"}}},
    {{"railway", "station", "subway", "barcelona"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "barcelona"}}},
    {{"railway", "station", "subway", "berlin"}, {{"railway", "station"}, {"station", "subway"}, {"city", "berlin"}}},
    {{"railway", "station", "subway", "berlin"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "berlin"}}},
    {{"railway", "station", "subway", "kiev"}, {{"railway", "station"}, {"station", "subway"}, {"city", "kiev"}}},
    {{"railway", "station", "subway", "kiev"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "kiev"}}},
    {{"railway", "station", "subway", "london"}, {{"railway", "station"}, {"station", "subway"}, {"city", "london"}}},
    {{"railway", "station", "subway", "london"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "london"}}},
    {{"railway", "station", "subway", "madrid"}, {{"railway", "station"}, {"station", "subway"}, {"city", "madrid"}}},
    {{"railway", "station", "subway", "madrid"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "madrid"}}},
    {{"railway", "station", "subway", "minsk"}, {{"railway", "station"}, {"station", "subway"}, {"city", "minsk"}}},
    {{"railway", "station", "subway", "minsk"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "minsk"}}},
    {{"railway", "station", "subway", "moscow"}, {{"railway", "station"}, {"station", "subway"}, {"city", "moscow"}}},
    {{"railway", "station", "subway", "moscow"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "moscow"}}},
    {{"railway", "station", "subway", "newyork"}, {{"railway", "station"}, {"station", "subway"}, {"city", "newyork"}}},
    {{"railway", "station", "subway", "newyork"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "newyork"}}},
    {{"railway", "station", "subway", "paris"}, {{"railway", "station"}, {"station", "subway"}, {"city", "paris"}}},
    {{"railway", "station", "subway", "paris"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "paris"}}},
    {{"railway", "station", "subway", "roma"}, {{"railway", "station"}, {"station", "subway"}, {"city", "roma"}}},
    {{"railway", "station", "subway", "roma"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "roma"}}},
    {{"railway", "station", "subway", "spb"}, {{"railway", "station"}, {"station", "subway"}, {"city", "spb"}}},
    {{"railway", "station", "subway", "spb"}, {{"railway", "station"}, {"transport", "subway"}, {"city", "spb"}}},
    {{"railway", "station", "subway"}, {{"railway", "station"}, {"station", "subway"}}},
    {{"railway", "station", "subway"}, {{"railway", "station"}, {"transport", "subway"}}},
    {{"railway", "subway", "bridge"}, {{"railway", "subway"}, {"bridge", "any_value"}}},
    //{{"railway", "subway", "tunnel"}, {{"railway", "subway"}, {"tunnel", "any_value"}}},
    {{"railway", "subway_entrance", "barcelona"}, {{"railway", "subway_entrance"}, {"city", "barcelona"}}},
    {{"railway", "subway_entrance", "berlin"}, {{"railway", "subway_entrance"}, {"city", "berlin"}}},
    {{"railway", "subway_entrance", "kiev"}, {{"railway", "subway_entrance"}, {"city", "kiev"}}},
    {{"railway", "subway_entrance", "london"}, {{"railway", "subway_entrance"}, {"city", "london"}}},
    {{"railway", "subway_entrance", "madrid"}, {{"railway", "subway_entrance"}, {"city", "madrid"}}},
    {{"railway", "subway_entrance", "minsk"}, {{"railway", "subway_entrance"}, {"city", "minsk"}}},
    {{"railway", "subway_entrance", "moscow"}, {{"railway", "subway_entrance"}, {"city", "moscow"}}},
    {{"railway", "subway_entrance", "newyork"}, {{"railway", "subway_entrance"}, {"city", "newyork"}}},
    {{"railway", "subway_entrance", "paris"}, {{"railway", "subway_entrance"}, {"city", "paris"}}},
    {{"railway", "subway_entrance", "roma"}, {{"railway", "subway_entrance"}, {"city", "roma"}}},
    {{"railway", "subway_entrance", "spb"}, {{"railway", "subway_entrance"}, {"city", "spb"}}},
    {{"railway", "tram", "bridge"}, {{"railway", "tram"}, {"bridge", "any_value"}}},
    {{"railway", "tram", "tunnel"}, {{"railway", "tram"}, {"tunnel", "any_value"}}},
    {{"shop", "car_repair", "tyres"}, {{"shop", "car_repair"}, {"service", "tyres"}}},
    {{"shop", "clothes"}, {{"shop", "clothes"}}},
    {{"shop", "clothes"}, {{"shop", "fashion"}}},
    {{"shop"}, {{"shop", "any_value"}}},
    {{"sport", "golf"}, {{"sport", "golf"}}},
    {{"sport", "golf"}, {{"sport", "miniature_golf"}}},
    {{"tourism", "artwork", "architecture"}, {{"tourism", "artwork"}, {"artwork_type", "architecture"}}},
    {{"tourism", "artwork", "architecture"}, {{"tourism", "artwork"}, {"type", "architecture"}}},
    {{"tourism", "artwork", "painting"}, {{"tourism", "artwork"}, {"artwork_type", "painting"}}},
    {{"tourism", "artwork", "painting"}, {{"tourism", "artwork"}, {"type", "painting"}}},
    {{"tourism", "artwork", "sculpture"}, {{"tourism", "artwork"}, {"artwork_type", "sculpture"}}},
    {{"tourism", "artwork", "sculpture"}, {{"tourism", "artwork"}, {"type", "sculpture"}}},
    {{"tourism", "artwork", "statue"}, {{"tourism", "artwork"}, {"artwork_type", "statue"}}},
    {{"tourism", "artwork", "statue"}, {{"tourism", "artwork"}, {"type", "statue"}}},
    {{"attraction", "animal"}, {{"attraction", "animal"}}},
    {{"tourism", "information", "board"}, {{"tourism", "information"}, {"information", "board"}}},
    {{"tourism", "information", "guidepost"}, {{"tourism", "information"}, {"information", "guidepost"}}},
    {{"tourism", "information", "map"}, {{"tourism", "information"}, {"information", "map"}}},
    {{"tourism", "information", "office"}, {{"tourism", "information"}, {"information", "office"}}},
    //{{"waterway", "canal", "tunnel"}, {{"waterway", "canal"}, {"tunnel", "any_value"}}},
    //{{"waterway", "river", "tunnel"}, {{"waterway", "river"}, {"tunnel", "any_value"}}},
    {{"waterway", "stream", "ephemeral"}, {{"waterway", "stream"}, {"intermittent", "ephemeral"}}},
    {{"waterway", "stream", "intermittent"}, {{"waterway", "stream"}, {"intermittent", "yes"}}},
  };

  for (auto const & type : complexTypes)
  {
    auto const params = GetFeatureBuilderParams(type.second);
    TEST_EQUAL(params.m_types.size(), 1, (type, params));
    TEST(params.IsTypeExist(GetType(type.first)), (type, params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_HighwayTypesConversion)
{
  using Type = std::vector<std::string>;
  std::vector<std::pair<Type, Tags>> const conversions = {
    {{"highway", "cycleway"}, {{"highway", "path"}, {"foot", "no"}, {"bicycle", "designated"}}},

    // Paved etc. paths to footways.
    {{"highway", "footway"}, {{"highway", "path"}, {"surface", "paved"}}},
    {{"highway", "footway"}, {{"highway", "path"}, {"surface", "paved"}, {"smoothness", "bad"}}},
    {{"highway", "footway"}, {{"highway", "path"}, {"surface", "compacted"}, {"smoothness", "intermediate"}}},
    {{"highway", "footway"}, {{"highway", "path"}, {"surface", "gravel"}, {"smoothness", "good"}}},
    {{"highway", "footway", "sidewalk"}, {{"highway", "path"}, {"surface", "gravel"}, {"footway", "sidewalk"}}},
    {{"highway", "footway"}, {{"highway", "path"}, {"smoothness", "good"}}},
    {{"highway", "footway", "crossing"}, {{"highway", "path"}, {"footway", "crossing"}}},
    {{"highway", "footway"}, {{"highway", "path"}, {"lit", "yes"}}},
    {{"highway", "footway"}, {{"highway", "path"}, {"segregated", "no"}}},
    // No conversion.
    {{"highway", "path"}, {{"highway", "path"}, {"surface", "unpaved"}}},
    {{"highway", "path"}, {{"highway", "path"}, {"surface", "compacted"}, {"smoothness", "bad"}}},
    {{"highway", "path"}, {{"highway", "path"}, {"surface", "gravel"}, {"tracktype", "grade3"}}},
    {{"highway", "path"}, {{"highway", "path"}, {"surface", "gravel"}, {"tracktype", "grade1"}, {"sac_scale", "hiking"}}},
    {{"highway", "path"}, {{"highway", "path"}, {"smoothness", "good"}, {"tracktype", "grade3"}}},
    {{"highway", "path"}, {{"highway", "path"}, {"tracktype", "grade3"}, {"footway", "sidewalk"}}},
    {{"highway", "path"}, {{"highway", "path"}, {"smoothness", "intermediate"}}},
    {{"highway", "path"}, {{"highway", "path"}, {"lit", "no"}}},

    // Unpaved etc. footways to paths.
    {{"highway", "path"}, {{"highway", "footway"}, {"surface", "unpaved"}}},
    {{"highway", "path"}, {{"highway", "footway"}, {"surface", "unpaved"}, {"smoothness", "excellent"}}},
    {{"highway", "path"}, {{"highway", "footway"}, {"surface", "compacted"}, {"smoothness", "bad"}}},
    {{"highway", "path"}, {{"highway", "footway"}, {"smoothness", "bad"}}},
    {{"highway", "path"}, {{"highway", "footway"}, {"sac_scale", "hiking"}}},
    {{"highway", "path"}, {{"highway", "footway"}, {"trail_visibility", "good"}}},
    {{"highway", "path"}, {{"highway", "footway"}, {"tracktype", "grade2"}, {"sac_scale", "hiking"}}},
    // TODO(@pastk): ford=* is converted to highway=ford via replaced_tags.txt; get rid of highway=ford
    {{"highway", "footway"}, {{"highway", "footway"}, {"ford", "stepping_stones"}}},
    {{"highway", "path"}, {{"highway", "footway"}, {"informal", "yes"}}},
    // No conversion.
    {{"highway", "footway"}, {{"highway", "footway"}, {"surface", "paved"}}},
    {{"highway", "footway"}, {{"highway", "footway"}, {"surface", "compacted"}, {"smoothness", "good"}}},
    {{"highway", "footway"}, {{"highway", "footway"}, {"smoothness", "good"}, {"tracktype", "grade2"}}},
    {{"highway", "footway"}, {{"highway", "footway"}, {"tracktype", "grade1"}, {"sac_scale", "hiking"}}},
    {{"highway", "footway", "sidewalk"}, {{"highway", "footway"}, {"tracktype", "grade2"}, {"footway", "sidewalk"}}},
    {{"highway", "footway"}, {{"highway", "footway"}, {"lit", "no"}}},
  };

  for (auto const & type : conversions)
  {
    auto const params = GetFeatureBuilderParams(type.second);
    TEST(params.IsTypeExist(GetType(type.first)), (type, params));
  }

  std::vector<std::pair<std::vector<Type>, Tags>> const complexConversions = {
    // Add an explicit footway to a segregated cycleway.
    {{{"highway", "cycleway"}, {"highway", "footway"}}, {{"highway", "cycleway"}, {"segregated", "yes"}}},
    {{{"highway", "cycleway"}, {"highway", "footway"}, {"hwtag", "yesfoot"}}, {{"highway", "cycleway"}, {"sidewalk", "right"}}},

    // Segregated path becomes cycleway + footway.
    {{{"highway", "cycleway"}, {"highway", "footway"}}, {{"highway", "path"}, {"segregated", "yes"}}},
    
    // A non-segregated cycleway becomes shared path/footway + bicycle=designated.
    {{{"highway", "footway", "bicycle"}, {"hwtag", "yesbicycle"}}, {{"highway", "cycleway"}, {"segregated", "no"}, {"foot", "designated"}}},
    {{{"highway", "path", "bicycle"}, {"hwtag", "yesbicycle"}, {"hwtag", "yesfoot"}, {"psurface", "unpaved_good"}}, {{"highway", "cycleway"}, {"foot", "yes"}, {"surface", "unpaved"}}},
  };

  for (auto const & type : complexConversions)
  {
    auto const & results = type.first;
    auto const params = GetFeatureBuilderParams(type.second);
    TEST_EQUAL(params.m_types.size(), results.size(), (type, params));
    for (auto const & result : results)
    {
      TEST(params.IsTypeExist(GetType(result)), (type, params));
    }
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_PathGrades)
{
  using Type = std::vector<std::string>;
  std::vector<std::pair<Type, Tags>> const conversions = {
    {{"highway", "path"}, {{"highway", "path"}, {"sac_scale", "mountain_hiking"}, {"trail_visibility", "intermediate"}}},
    {{"highway", "path"}, {{"highway", "path"}, {"sac_scale", "unsupported_value"}, {"trail_visibility", "unsupported_value"}}},

    {{"highway", "path", "difficult"}, {{"highway", "path"}, {"sac_scale", "demanding_mountain_hiking"}, {"trail_visibility", "excellent"}}},
    {{"highway", "path", "difficult"}, {{"highway", "path"}, {"trail_visibility", "bad"}}},    

    {{"highway", "path", "expert"}, {{"highway", "path"}, {"sac_scale", "alpine_hiking"}}},
    {{"highway", "path", "expert"}, {{"highway", "path"}, {"trail_visibility", "horrible"}}},
    {{"highway", "path", "expert"}, {{"highway", "path"}, {"sac_scale", "difficult_alpine_hiking"}, {"trail_visibility", "no"}}},
  };

  for (auto const & type : conversions)
  {
    auto const params = GetFeatureBuilderParams(type.second);
    TEST(params.IsTypeExist(GetType(type.first)), (type, params));
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_MultipleComplexTypesSmoke)
{
  using Type = std::vector<std::string>;
  std::vector<std::pair<std::vector<Type>, Tags>> const complexTypes = {
    {{{"amenity", "parking"}, {"fee", "no"}}, {{"amenity", "parking"}, {"fee", "no"}}},
    {{{"amenity", "parking", "fee"}, {"fee", "yes"}}, {{"amenity", "parking"}, {"fee", "any_value"}}},
    {{{"amenity", "parking", "lane", "fee"}, {"fee", "yes"}}, {{"amenity", "parking"}, {"parking", "lane"}, {"fee", "any_value"}}},
    {{{"amenity", "parking", "multi-storey", "fee"}, {"fee", "yes"}}, {{"amenity", "parking"}, {"parking", "multi-storey"}, {"fee", "any_value"}}},
    {{{"amenity", "parking", "street_side", "fee"}, {"fee", "yes"}}, {{"amenity", "parking"}, {"parking", "street_side"}, {"fee", "any_value"}}},
    {{{"amenity", "parking", "underground", "fee"}, {"fee", "yes"}}, {{"amenity", "parking"}, {"parking", "underground"}, {"fee", "any_value"}}},
  };

  for (auto const & type : complexTypes)
  {
    auto const & results = type.first;
    auto const params = GetFeatureBuilderParams(type.second);
    TEST_EQUAL(params.m_types.size(), results.size(), (type, params));
    for (auto const & result : results)
    {
      TEST(params.IsTypeExist(GetType(result)), (type, params));
    }
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Mtb_Rating)
{
  using Type = std::vector<std::string>;
  std::vector<std::pair<Type, Tags>> const conversions = {
    {{"highway", "cycleway", "easy"}, {{"highway", "cycleway"}, {"mtb:scale", "0"}, {"smoothness", "good"}}},
    {{"highway", "cycleway", "easy"}, {{"highway", "cycleway"}, {"mtb:scale", "S0"}}},
    {{"highway", "cycleway", "easy"}, {{"highway", "cycleway"}, {"mtb:scale:imba", "1"}}},    
    {{"highway", "cycleway", "easy"}, {{"highway", "cycleway"}, {"smoothness", "bad"}}},    

    {{"highway", "cycleway", "intermediate"}, {{"highway", "cycleway"}, {"mtb:scale", "1"}, {"smoothness", "good"}}},
    {{"highway", "cycleway", "intermediate"}, {{"highway", "cycleway"}, {"mtb:scale", "S2"}}},
    {{"highway", "cycleway", "intermediate"}, {{"highway", "cycleway"}, {"mtb:scale:imba", "2"}}},    
    {{"highway", "cycleway", "intermediate"}, {{"highway", "cycleway"}, {"smoothness", "horrible"}}},    

    {{"highway", "cycleway", "difficult"}, {{"highway", "cycleway"}, {"mtb:scale", "3"}, {"smoothness", "very_bad"}}},
    {{"highway", "cycleway", "difficult"}, {{"highway", "cycleway"}, {"mtb:scale", "S3"}}},
    {{"highway", "cycleway", "difficult"}, {{"highway", "cycleway"}, {"mtb:scale:imba", "3"}, {"smoothness", "bad"}}},    
    {{"highway", "cycleway", "difficult"}, {{"highway", "cycleway"}, {"smoothness", "very_horrible"}}},    

    {{"highway", "cycleway", "expert"}, {{"highway", "cycleway"}, {"mtb:scale", "4"}, {"smoothness", "very_bad"}}},
    {{"highway", "cycleway", "expert"}, {{"highway", "cycleway"}, {"mtb:scale", "S5"}}},
    {{"highway", "cycleway", "expert"}, {{"highway", "cycleway"}, {"mtb:scale:imba", "4"}, {"smoothness", "bad"}}} 

  };

  for (auto const & type : conversions)
  {
    auto const params = GetFeatureBuilderParams(type.second);
    TEST(params.IsTypeExist(GetType(type.first)), (type, params));
  }
}

}  // namespace osm_type_test
