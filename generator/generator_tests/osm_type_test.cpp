#include "../../testing/testing.hpp"

#include "../xml_element.hpp"
#include "../osm2type.hpp"

#include "../../indexer/feature_data.hpp"
#include "../../indexer/classificator.hpp"
#include "../../indexer/classificator_loader.hpp"


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
