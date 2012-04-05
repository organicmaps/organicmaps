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
