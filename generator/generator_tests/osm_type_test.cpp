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

UNIT_CLASS_TEST(TestWithClassificator, OsmType_SimpleTypesSmoke)
{
  Tags const simpleTypes = {
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
    // {"barrier", "cycle_barrier"},
    // {"boundary", "administrative"},
    // {"communication", "line"},
    // {"earthquake:damage", "spontaneous_camp"},
    // {"highway", "platform"},
    // {"highway", "traffic_signals"},
    // {"historic", "battlefield"},
    // {"landuse", "brownfield"},
    // {"landuse", "farm"},
    // {"landuse", "farmyard"},
    // {"landuse", "greenfield"},
    // {"landuse", "greenhouse_horticulture"},
    // {"landuse", "residential"},
    // {"landuse", "retail"},
    // {"man_made", "cairn"},
    // {"man_made", "pipeline"},
    // {"man_made", "surveillance"},
    // {"man_made", "tower"},
    // {"man_made", "wastewater_plant"},
    // {"man_made", "water_tower"},
    // {"man_made", "water_well"},
    // {"mapswithme", "grid"},
    // {"military", "bunker"},
    // {"natural", "cliff"},
    // {"natural", "meadow"},
    // {"natural", "orchard"},
    // {"natural", "tree"},
    // {"natural", "tree_row"},
    // {"natural", "vineyard"},
    // {"noexit", "motor_vehicle"},
    // {"olympics", "attraction"},
    // {"olympics", "bike_sport"},
    // {"olympics", "live_site"},
    // {"olympics", "official_building"},
    // {"olympics", "stadium"},
    // {"olympics", "stadium_main"},
    // {"olympics", "transport_airport"},
    // {"olympics", "transport_boat"},
    // {"olympics", "transport_bus"},
    // {"olympics", "transport_cable"},
    // {"olympics", "transport_railway"},
    // {"olympics", "transport_subway"},
    // {"olympics", "transport_tram"},
    // {"olympics", "water_sport"},
    // {"place", "county"},
    // {"power", "generator"},
    // {"power", "minor_line"},
    // {"power", "pole"},
    // {"railway", "crossing"},
    // {"railway", "proposed"},
    // {"railway", "razed"},
    // {"railway", "siding"},
    // {"railway", "spur"},
    // {"railway", "subway"},
    // {"traffic_calming", "bump"},
    // {"traffic_calming", "hump"},
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
    {"amenity", "embassy"},
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
    {"amenity", "recycling"},
    {"amenity", "restaurant"},
    {"amenity", "school"},
    {"amenity", "shelter"},
    {"amenity", "shower"},
    {"amenity", "speed_trap"},
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
    {"barrier", "city_wall"},
    {"barrier", "entrance"},
    {"barrier", "fence"},
    {"barrier", "gate"},
    {"barrier", "hedge"},
    {"barrier", "lift_gate"},
    {"barrier", "retaining_wall"},
    {"barrier", "stile"},
    {"barrier", "toll_booth"},
    {"barrier", "wall"},
    {"boundary", "national_park"},
    {"building", "has_parts"},
    {"building", "train_station"},
    {"craft", "brewery"},
    {"craft", "carpenter"},
    {"craft", "electrician"},
    {"craft", "gardener"},
    {"craft", "hvac"},
    {"craft", "metal_construction"},
    {"craft", "painter"},
    {"craft", "photographer"},
    {"craft", "plumber"},
    {"craft", "shoemaker"},
    {"craft", "tailor"},
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
    {"highway", "bridleway"},
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
    {"highway", "proposed"},
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
    {"historic", "citywalls"},
    {"historic", "fort"},
    {"historic", "memorial"},
    {"historic", "monument"},
    {"historic", "museum"},
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
    {"junction", "roundabout"},
    {"landuse", "allotments"},
    {"landuse", "basin"},
    {"landuse", "cemetery"},
    {"landuse", "churchyard"},
    {"landuse", "commercial"},
    {"landuse", "construction"},
    {"landuse", "farmland"},
    {"landuse", "field"},
    {"landuse", "forest"},
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
    {"leisure", "landscape_reserve"},
    {"leisure", "marina"},
    {"leisure", "nature_reserve"},
    {"leisure", "park"},
    {"leisure", "pitch"},
    {"leisure", "playground"},
    {"leisure", "recreation_ground"},
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
    {"man_made", "lighthouse"},
    {"man_made", "pier"},
    {"man_made", "water_tap"},
    {"man_made", "windmill"},
    {"man_made", "works"},
    {"natural", "bare_rock"},
    {"natural", "bay"},
    {"natural", "beach"},
    {"natural", "cape"},
    {"natural", "cave_entrance"},
    {"natural", "coastline"},
    {"natural", "geyser"},
    {"natural", "glacier"},
    {"natural", "grassland"},
    {"natural", "heath"},
    {"natural", "hot_spring"},
    {"natural", "lake"},
    {"natural", "land"},
    {"natural", "peak"},
    {"natural", "pond"},
    {"natural", "rock"},
    {"natural", "salt_pond"},
    {"natural", "scrub"},
    {"natural", "spring"},
    {"natural", "volcano"},
    {"natural", "water"},
    {"natural", "wetland"},
    {"natural", "wood"},
    {"office", "company"},
    {"office", "estate_agent"},
    {"office", "government"},
    {"office", "insurance"},
    {"office", "lawyer"},
    {"office", "ngo"},
    {"office", "telecommunication"},
    {"piste:lift", "j-bar"},
    {"piste:lift", "magic_carpet"},
    {"piste:lift", "platter"},
    {"piste:lift", "rope_tow"},
    {"piste:lift", "t-bar"},
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
    {"railway", "rail"},
    {"railway", "station"},
    {"railway", "subway_entrance"},
    {"railway", "tram"},
    {"railway", "tram_stop"},
    {"railway", "yard"},
    {"route", "ferry"},
    {"route", "shuttle_train"},
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
    {"shop", "department_store"},
    {"shop", "doityourself"},
    {"shop", "dry_cleaning"},
    {"shop", "electronics"},
    {"shop", "erotic"},
    {"shop", "fabric"},
    {"shop", "florist"},
    {"shop", "funeral_directors"},
    {"shop", "furniture"},
    {"shop", "garden_centre"},
    {"shop", "gift"},
    {"shop", "greengrocer"},
    {"shop", "hairdresser"},
    {"shop", "hardware"},
    {"shop", "jewelry"},
    {"shop", "kiosk"},
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
    {"shop", "pawnbroker"},
    {"shop", "pet"},
    {"shop", "photo"},
    {"shop", "seafood"},
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
    {"sponsored", "booking"},
    {"sponsored", "holiday"},
    {"sponsored", "opentable"},
    {"sponsored", "partner1"},
    {"sponsored", "partner10"},
    {"sponsored", "partner11"},
    {"sponsored", "partner12"},
    {"sponsored", "partner13"},
    {"sponsored", "partner14"},
    {"sponsored", "partner15"},
    {"sponsored", "partner16"},
    {"sponsored", "partner17"},
    {"sponsored", "partner18"},
    {"sponsored", "partner19"},
    {"sponsored", "partner2"},
    {"sponsored", "partner20"},
    {"sponsored", "partner3"},
    {"sponsored", "partner4"},
    {"sponsored", "partner5"},
    {"sponsored", "partner6"},
    {"sponsored", "partner7"},
    {"sponsored", "partner8"},
    {"sponsored", "partner9"},
    {"sponsored", "promo_catalog"},
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
    {"sport", "football"},
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
    {"tourism", "resort"},
    {"tourism", "theme_park"},
    {"tourism", "viewpoint"},
    {"tourism", "wilderness_hut"},
    {"tourism", "zoo"},
    {"waterway", "canal"},
    {"waterway", "dam"},
    {"waterway", "ditch"},
    {"waterway", "dock"},
    {"waterway", "drain"},
    {"waterway", "lock"},
    {"waterway", "lock_gate"},
    {"waterway", "river"},
    {"waterway", "riverbank"},
    {"waterway", "stream"},
    {"waterway", "waterfall"},
    {"waterway", "weir"},
    {"wheelchair", "limited"},
    {"wheelchair", "no"},
    {"wheelchair", "yes"},
  };

  for (auto const & type : simpleTypes)
  {
     auto const params = GetFeatureBuilderParams({type});
     TEST_EQUAL(params.m_types.size(), 1, (type, params));
     TEST(params.IsTypeExist(classif().GetTypeByPath({type.m_key, type.m_value})), (type, params));
  }
}
