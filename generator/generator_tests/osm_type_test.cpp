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

  uint32_t GetType(char const * arr[2])
  {
    vector<string> path;
    path.push_back(arr[0]);
    path.push_back(arr[1]);
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

  Classificator & c = classif();
  TEST(params.IsTypeExist(c.GetTypeByPath(vector<string>(arr[3], arr[3] + 2))), ());
  TEST(params.IsTypeExist(c.GetTypeByPath(vector<string>(arr[4], arr[4] + 1))), ());

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

  Classificator & c = classif();
  char const * arrT[] = { "building", "address" };
  TEST(params.IsTypeExist(c.GetTypeByPath(vector<string>(arrT, arrT + 2))), ());

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

  Classificator & c = classif();
  char const * arrT[] = { "place", "state", "USA" };
  TEST(params.IsTypeExist(c.GetTypeByPath(vector<string>(arrT, arrT + 3))), ());

  string s;
  TEST(params.name.GetString(0, s), ());
  TEST_EQUAL(s, "California", ());
  TEST_GREATER(params.rank, 1, ());
}
