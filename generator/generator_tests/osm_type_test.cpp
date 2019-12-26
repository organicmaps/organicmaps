#include "testing/testing.hpp"

#include "generator/generator_tests/types_helper.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm2type.hpp"
#include "generator/tag_admixer.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace generator::tests_support;
using namespace tests;

using Tags = std::vector<OsmElement::Tag>;

namespace
{
  void DumpTypes(std::vector<uint32_t> const & v)
  {
    Classificator const & c = classif();
    for (size_t i = 0; i < v.size(); ++i)
      std::cout << c.GetFullObjectName(v[i]) << std::endl;
  }

  void DumpParsedTypes(Tags const & tags)
  {
    OsmElement e;
    FillXmlElement(tags, &e);

    FeatureBuilderParams params;
    ftype::GetNameAndType(&e, params);

    DumpTypes(params.m_types);
  }

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

  FeatureBuilderParams GetFeatureBuilderParams(Tags const & tags)
  {
    OsmElement e;
    FillXmlElement(tags, &e);
    FeatureBuilderParams params;

    ftype::GetNameAndType(&e, params);
    return params;
  }
}  // namespace

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

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Check)
{
  Tags const tags1 = {
    { "highway", "primary" },
    { "motorroad", "yes" },
    { "name", "Каширское шоссе" },
    { "oneway", "yes" }
  };

  Tags const tags2 = {
    { "highway", "primary" },
    { "name", "Каширское шоссе" },
    { "oneway", "-1" },
    { "motorroad", "yes" }
  };

  Tags const tags3 = {
    { "admin_level", "4" },
    { "border_type", "state" },
    { "boundary", "administrative" }
  };

  Tags const tags4 = {
    { "border_type", "state" },
    { "admin_level", "4" },
    { "boundary", "administrative" }
  };

  DumpParsedTypes(tags1);
  DumpParsedTypes(tags2);
  DumpParsedTypes(tags3);
  DumpParsedTypes(tags4);
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

  std::string s;
  params.name.GetString(0, s);
  TEST_EQUAL(s, "Гимназия 15", ());

  TEST_EQUAL(params.house.Get(), "84", ());
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Address)
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
  TEST(params.IsTypeExist(GetType({"building", "address"})), ());

  TEST_EQUAL(params.house.Get(), "223/5", ());
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

  std::string s;
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

    OsmElement e;
    FillXmlElement(tags, &e);

    TagReplacer tagReplacer(GetPlatform().ResourcesDir() + REPLACED_TAGS_FILE);
    tagReplacer.Process(e);

    FeatureBuilderParams params;
    ftype::GetNameAndType(&e, params);

    TEST_EQUAL(params.m_types.size(), 7, (params));

    TEST(params.IsTypeExist(GetType({"building"})), ());
    TEST(params.IsTypeExist(GetType({"amenity", "atm"})), ());
    TEST(params.IsTypeExist(GetType({"shop"})), ());
    TEST(params.IsTypeExist(GetType({"amenity", "restaurant"})), ());
    TEST(params.IsTypeExist(GetType({"tourism", "hotel"})), ());
    TEST(params.IsTypeExist(GetType({"amenity", "toilets"})), ());
    TEST(params.IsTypeExist(GetType({"amenity", "drinking_water"})), ());
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
      { "admin_level", "6" },
      { "capital", "yes" },
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
      { "railway", "rail" },
      { "access", "private" },
      { "oneway", "true" },
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(params.IsTypeExist(GetType({"railway", "rail"})), ());
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
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 6, (params));
    TEST(params.IsTypeExist(GetType({"highway", "primary"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "oneway"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "private"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "nofoot"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesbicycle"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "bidir_bicycle"})), ());
  }

  {
    Tags const tags = {
        {"foot", "yes"},
        {"cycleway", "lane"},
        {"highway", "primary"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 3, (params));
    TEST(params.IsTypeExist(GetType({"highway", "primary"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesfoot"})), ());
    TEST(params.IsTypeExist(GetType({"hwtag", "yesbicycle"})), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Surface)
{
  TestSurfaceTypes("asphalt", "", "", "paved_good");
  TestSurfaceTypes("asphalt", "bad", "", "paved_bad");
  TestSurfaceTypes("asphalt", "", "0", "paved_bad");
  TestSurfaceTypes("paved", "", "2", "paved_good");
  TestSurfaceTypes("", "excellent", "", "paved_good");
  TestSurfaceTypes("wood", "", "", "paved_bad");
  TestSurfaceTypes("wood", "good", "", "paved_good");
  TestSurfaceTypes("wood", "", "3", "paved_good");
  TestSurfaceTypes("unpaved", "", "", "unpaved_good");
  TestSurfaceTypes("mud", "", "", "unpaved_bad");
  TestSurfaceTypes("", "bad", "", "unpaved_good");
  TestSurfaceTypes("", "horrible", "", "unpaved_bad");
  TestSurfaceTypes("ground", "", "1", "unpaved_bad");
  TestSurfaceTypes("mud", "", "3", "unpaved_good");
  TestSurfaceTypes("unknown", "", "", "unpaved_good");
  TestSurfaceTypes("", "unknown", "", "unpaved_good");
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Ferry)
{
  routing::CarModel const & carModel = routing::CarModel::AllLimitsInstance();

  Tags const tags = {
    { "motorcar", "yes" },
    { "highway", "primary" },
    { "bridge", "yes" },
    { "route", "ferry" },
  };

  auto const params = GetFeatureBuilderParams(tags);

  TEST_EQUAL(params.m_types.size(), 3, (params));

  uint32_t type = GetType({"highway", "primary", "bridge"});
  TEST(params.IsTypeExist(type), ());
  TEST(carModel.IsRoadType(type), ());

  type = GetType({"route", "ferry", "motorcar"});
  TEST(params.IsTypeExist(type), (params));
  TEST(carModel.IsRoadType(type), ());

  type = GetType({"route", "ferry"});
  TEST(!params.IsTypeExist(type), ());
  TEST(carModel.IsRoadType(type), ());

  type = GetType({"hwtag", "yescar"});
  TEST(params.IsTypeExist(type), ());
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_YesCarNoCar)
{
  routing::CarModel const & carModel = routing::CarModel::AllLimitsInstance();

  {
    Tags const tags = {
        {"highway", "secondary"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 1, (params));
    TEST(!params.IsTypeExist(carModel.GetNoCarTypeForTesting()), ());
    TEST(!params.IsTypeExist(carModel.GetYesCarTypeForTesting()), ());
  }

  {
    Tags const tags = {
        {"highway", "cycleway"},
        {"motorcar", "yes"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(!params.IsTypeExist(carModel.GetNoCarTypeForTesting()), ());
    TEST(params.IsTypeExist(carModel.GetYesCarTypeForTesting()), ());
  }

  {
    Tags const tags = {
        {"highway", "secondary"},
        {"motor_vehicle", "no"},
    };

    auto const params = GetFeatureBuilderParams(tags);

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(carModel.GetNoCarTypeForTesting()), ());
    TEST(!params.IsTypeExist(carModel.GetYesCarTypeForTesting()), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, OsmType_Boundary)
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
  std::string name;
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

    OsmElement e;
    FillXmlElement(tags, &e);

    TagReplacer tagReplacer(GetPlatform().ResourcesDir() + REPLACED_TAGS_FILE);
    tagReplacer.Process(e);

    FeatureBuilderParams params;
    ftype::GetNameAndType(&e, params);

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

  std::string name;
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

    TEST_EQUAL(params.m_types.size(), 2, (params));
    TEST(params.IsTypeExist(GetType({"amenity", "parking", "private"})), (params));
    TEST(params.IsTypeExist(GetType({"amenity", "parking", "fee"})), (params));
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
