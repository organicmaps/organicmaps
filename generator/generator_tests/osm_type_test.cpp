#include "../../testing/testing.hpp"

#include "../xml_element.hpp"
#include "../osm2type.hpp"

#include "../../indexer/feature_data.hpp"
#include "../../indexer/classificator.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../../std/iostream.hpp"


namespace
{
  void FillXmlElement(char const * arr[][2], size_t count, XMLElement * p)
  {
    p->parent = 0;
    for (size_t i = 0; i < count; ++i)
    {
      p->childs.push_back(XMLElement());
      p->childs.back().name = "tag";
      p->childs.back().attrs["k"] = arr[i][0];
      p->childs.back().attrs["v"] = arr[i][1];
    }
  }

  template <size_t N> uint32_t GetType(char const * (&arr)[N])
  {
    vector<string> path(arr, arr + N);
    return classif().GetTypeByPath(path);
  }
}

UNIT_TEST(OsmType_SkipDummy)
{
  classificator::Load();

  char const * arr[][2] = {
    { "abutters", "residential" },
    { "highway", "primary" },
    { "osmarender:renderRef", "no" },
    { "ref", "E51" }
  };

  XMLElement e;
  FillXmlElement(arr, ARRAY_SIZE(arr), &e);

  FeatureParams params;
  ftype::GetNameAndType(&e, params);

  TEST_EQUAL ( params.m_Types.size(), 1, () );
  TEST_EQUAL ( params.m_Types[0], GetType(arr[1]), () );
}


namespace
{
  void DumpTypes(vector<uint32_t> const & v)
  {
    Classificator const & c = classif();
    for (size_t i = 0; i < v.size(); ++i)
      cout << c.GetFullObjectName(v[i]) << endl;
  }

  void DumpParsedTypes(char const * arr[][2], size_t count)
  {
    XMLElement e;
    FillXmlElement(arr, count, &e);

    FeatureParams params;
    ftype::GetNameAndType(&e, params);

    DumpTypes(params.m_Types);
  }
}

UNIT_TEST(OsmType_Check)
{
  classificator::Load();

  char const * arr1[][2] = {
    { "highway", "primary" },
    { "motorroad", "yes" },
    { "name", "Каширское шоссе" },
    { "oneway", "yes" }
  };

  char const * arr2[][2] = {
    { "highway", "primary" },
    { "name", "Каширское шоссе" },
    { "oneway", "yes" },
    { "motorroad", "yes" }
  };

  char const * arr3[][2] = {
    { "admin_level", "4" },
    { "border_type", "state" },
    { "boundary", "administrative" }
  };

  char const * arr4[][2] = {
    { "border_type", "state" },
    { "admin_level", "4" },
    { "boundary", "administrative" }
  };

  DumpParsedTypes(arr1, ARRAY_SIZE(arr1));
  DumpParsedTypes(arr2, ARRAY_SIZE(arr2));
  DumpParsedTypes(arr3, ARRAY_SIZE(arr3));
  DumpParsedTypes(arr4, ARRAY_SIZE(arr4));
}

UNIT_TEST(OsmType_Combined)
{
  char const * arr[][2] = {
    { "addr:housenumber", "84" },
    { "addr:postcode", "220100" },
    { "addr:street", "ул. Максима Богдановича" },
    { "amenity", "school" },
    { "building", "yes" },
    { "name", "Гимназия 15" }
  };

  XMLElement e;
  FillXmlElement(arr, ARRAY_SIZE(arr), &e);

  FeatureParams params;
  ftype::GetNameAndType(&e, params);

  TEST(params.IsTypeExist(GetType(arr[3])), ());
  char const * arrT[] = { "building" };
  TEST(params.IsTypeExist(GetType(arrT)), ());

  string s;
  params.name.GetString(0, s);
  TEST_EQUAL(s, arr[5][1], ());

  TEST_EQUAL(params.house.Get(), "84", ());
}

UNIT_TEST(OsmType_Address)
{
  char const * arr[][2] = {
    { "addr:conscriptionnumber", "223" },
    { "addr:housenumber", "223/5" },
    { "addr:postcode", "11000" },
    { "addr:street", "Řetězová" },
    { "addr:streetnumber", "5" },
    { "source:addr", "uir_adr" },
    { "uir_adr:ADRESA_KOD", "21717036" }
  };

  XMLElement e;
  FillXmlElement(arr, ARRAY_SIZE(arr), &e);

  FeatureParams params;
  ftype::GetNameAndType(&e, params);

  char const * arrT[] = { "building", "address" };
  TEST(params.IsTypeExist(GetType(arrT)), ());

  TEST_EQUAL(params.house.Get(), "223/5", ());
}

UNIT_TEST(OsmType_PlaceState)
{
  char const * arr[][2] = {
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

  XMLElement e;
  FillXmlElement(arr, ARRAY_SIZE(arr), &e);

  FeatureParams params;
  ftype::GetNameAndType(&e, params);

  char const * arrT[] = { "place", "state", "USA" };
  TEST(params.IsTypeExist(GetType(arrT)), ());

  string s;
  TEST(params.name.GetString(0, s), ());
  TEST_EQUAL(s, "California", ());
  TEST_GREATER(params.rank, 1, ());
}

UNIT_TEST(OsmType_AlabamaRiver)
{
  char const * arr1[][2] = {
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

  char const * arr2[][2] = {
    { "destination", "Ohio River" },
    { "name", "Tennessee River" },
    { "type", "waterway" },
    { "waterway", "river" }
  };

  char const * arr3[][2] = {
    { "name", "Tennessee River" },
    { "network", "inland waterways" },
    { "route", "boat" },
    { "ship", "yes" },
    { "type", "route" }
  };

  XMLElement e;
  FillXmlElement(arr1, ARRAY_SIZE(arr1), &e);
  FillXmlElement(arr2, ARRAY_SIZE(arr2), &e);
  FillXmlElement(arr3, ARRAY_SIZE(arr3), &e);

  FeatureParams params;
  ftype::GetNameAndType(&e, params);

  char const * arrT[] = { "waterway", "river" };
  TEST(params.IsTypeExist(GetType(arrT)), ());
  TEST_EQUAL(params.m_Types.size(), 1, ());
}

UNIT_TEST(OsmType_Synonyms)
{
  // Smoke test.
  {
    char const * arr[][2] = {
      { "building", "yes" },
      { "shop", "yes" },
      { "atm", "yes" }
    };

    XMLElement e;
    FillXmlElement(arr, ARRAY_SIZE(arr), &e);

    FeatureParams params;
    ftype::GetNameAndType(&e, params);

    char const * arrT1[] = { "building" };
    char const * arrT2[] = { "amenity", "atm" };
    char const * arrT3[] = { "shop" };
    TEST_EQUAL(params.m_Types.size(), 3, ());
    TEST(params.IsTypeExist(GetType(arrT1)), ());
    TEST(params.IsTypeExist(GetType(arrT2)), ());
    TEST(params.IsTypeExist(GetType(arrT3)), ());
  }

  // Duplicating test.
  {
    char const * arr[][2] = {
      { "amenity", "atm" },
      { "atm", "yes" }
    };

    XMLElement e;
    FillXmlElement(arr, ARRAY_SIZE(arr), &e);

    FeatureParams params;
    ftype::GetNameAndType(&e, params);

    char const * arrT[] = { "amenity", "atm" };
    TEST_EQUAL(params.m_Types.size(), 1, ());
    TEST(params.IsTypeExist(GetType(arrT)), ());
  }

  // "NO" tag test.
  {
    char const * arr[][2] = {
      { "building", "yes" },
      { "shop", "no" },
      { "atm", "no" }
    };

    XMLElement e;
    FillXmlElement(arr, ARRAY_SIZE(arr), &e);

    FeatureParams params;
    ftype::GetNameAndType(&e, params);

    char const * arrT[] = { "building" };
    TEST_EQUAL(params.m_Types.size(), 1, ());
    TEST(params.IsTypeExist(GetType(arrT)), ());
  }
}
