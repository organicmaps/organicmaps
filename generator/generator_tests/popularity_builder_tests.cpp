#include "testing/testing.hpp"

#include "generator/feature_generator.hpp"
#include "generator/generator_tests/common.hpp"
#include "generator/popularity.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/geo_object_id.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <vector>

using namespace generator;
using namespace generator::popularity;

namespace generator_tests
{
class TestPopularityBuilder
{
public:
  TestPopularityBuilder()
  {
    classificator::Load();
  }

  void GetType() const
  {
    {
      FeatureBuilder1 fb;
      auto const type = m_cl.GetTypeByPath({"tourism", "museum"});
      auto const checkableType =  m_cl.GetReadableObjectName(type);
      fb.AddType(m_cl.GetTypeByPath({"building"}));
      fb.AddType(type);
      TEST_EQUAL(PopularityBuilder::GetType(fb), checkableType, ());
    }

    {
      FeatureBuilder1 fb;
      fb.AddType(m_cl.GetTypeByPath({"building"}));
      TEST_EQUAL(PopularityBuilder::GetType(fb), "", ());
    }

    {
      FeatureBuilder1 fb;
      TEST_EQUAL(PopularityBuilder::GetType(fb), "", ());
    }
  }

  static void GetFeatureName()
  {
    {
      FeatureBuilder1 fb;
      std::string checkableName = "Фича";
      fb.AddName("ru", checkableName);
      fb.AddName("en", "Feature");
      TEST_EQUAL(PopularityBuilder::GetFeatureName(fb), checkableName, ());
    }

    {
      FeatureBuilder1 fb;
      std::string checkableName = "Feature";
      fb.AddName("en", checkableName);
      fb.AddName("default", "Fonctionnalité");
      TEST_EQUAL(PopularityBuilder::GetFeatureName(fb), checkableName, ());
    }

    {
      FeatureBuilder1 fb;
      std::string checkableName = "Fonctionnalité";
      fb.AddName("default", checkableName);
      TEST_EQUAL(PopularityBuilder::GetFeatureName(fb), checkableName, ());
    }

    {
      FeatureBuilder1 fb;
      fb.AddName("fr", "Fonctionnalité");
      TEST_EQUAL(PopularityBuilder::GetFeatureName(fb), "", ());
    }

    {
      FeatureBuilder1 fb;
      TEST_EQUAL(PopularityBuilder::GetFeatureName(fb), "", ());
    }
  }

  void FindPointParent() const
  {
    auto const filtered = FilterPoint(m_testSet);
    std::map<std::string, FeatureBuilder1> pointMap;
    for (auto const & f : filtered)
      pointMap.emplace(f.GetName(), f);

    auto const filteredArea = FilterArea(m_testSet);
    auto geomPlaces = MakePopularityGeomPlaces(filteredArea);
    auto const nameToNode = GetPlacesMap(geomPlaces);
    auto const m = PopularityBuilder::GetAreaMap(geomPlaces);
    auto const tree = PopularityBuilder::MakeTree4d(geomPlaces);

    {
      auto const & ft = pointMap.at("1_3_4");
      auto const parent = PopularityBuilder::FindPointParent(ft.GetKeyPoint(), m, tree);
      TEST(parent, ());
      TEST_EQUAL(*parent, nameToNode.at("1_3")->GetData().GetId(), ());
    }

    {
      auto const & ft = pointMap.at("1_3_5");
      auto const parent = PopularityBuilder::FindPointParent(ft.GetKeyPoint(), m, tree);
      TEST(parent, ());
      TEST_EQUAL(*parent, nameToNode.at("1_3")->GetData().GetId(), ());
    }

    {
      m2::PointD bad{1000.0, 1000.0};
      auto const parent = PopularityBuilder::FindPointParent(bad, m, tree);
      TEST(!parent, ());
    }
  }

  void FindPopularityGeomPlaceParent() const
  {
    auto const filtered = FilterArea(m_testSet);
    auto geomPlaces = MakePopularityGeomPlaces(filtered);
    auto const nameToNode = GetPlacesMap(geomPlaces);
    auto const m = PopularityBuilder::GetAreaMap(geomPlaces);
    auto const tree = PopularityBuilder::MakeTree4d(geomPlaces);

    {
      auto const t = PopularityBuilder::FindPopularityGeomPlaceParent(nameToNode.at("1_2")->GetData(), m, tree);
      TEST(t, ());
      TEST_EQUAL(*t, nameToNode.at("1"), ());
    }

    {
      auto const t = PopularityBuilder::FindPopularityGeomPlaceParent(nameToNode.at("1_3")->GetData(), m, tree);
      TEST(t, ());
      TEST_EQUAL(*t, nameToNode.at("1"), ());
    }

    {
      auto const t = PopularityBuilder::FindPopularityGeomPlaceParent(nameToNode.at("1")->GetData(), m, tree);
      TEST(!t, ());
    }
  }

  void GetAreaMap() const
  {
    {
      auto const filtered = FilterArea(m_testSet);
      auto const geomPlaces = MakePopularityGeomPlaces(filtered);
      std::map<base::GeoObjectId, PopularityBuilder::Node::Ptr> checkableMap;
      for (auto const & n : geomPlaces)
        checkableMap.emplace(n->GetData().GetId(), n);

      auto const m = PopularityBuilder::GetAreaMap(geomPlaces);
      TEST_EQUAL(m.size(), checkableMap.size(), ());
      for (auto const & p : checkableMap)
      {
        TEST_EQUAL(m.count(p.first), 1, ());
        TEST_EQUAL(p.second, m.at(p.first), ());
      }
    }

    {
      auto const m = PopularityBuilder::GetAreaMap({});
      TEST(m.empty(), ());
    }
  }

  void MakeTree4d() const
  {
    {
      std::set<base::GeoObjectId> checkableIds;
      auto const filtered = FilterArea(m_testSet);
      auto const geomPlaces = MakePopularityGeomPlaces(filtered);
      for (auto const & node : geomPlaces)
        checkableIds.insert(node->GetData().GetId());

      auto const tree = PopularityBuilder::MakeTree4d(geomPlaces);
      std::set<base::GeoObjectId> ids;
      tree.ForEach([&](auto const & id) {
        ids.insert(id);
      });

      TEST_EQUAL(checkableIds, ids, ());
    }

    {
      std::set<base::GeoObjectId> checkableIds;
      auto const tree = PopularityBuilder::MakeTree4d({});
      std::set<base::GeoObjectId> ids;
      tree.ForEach([&](auto const & id) {
        ids.insert(id);
      });

      TEST_EQUAL(checkableIds, ids, ());
    }
  }

  void LinkGeomPlaces() const
  {
    auto const filtered = FilterArea(m_testSet);
    auto geomPlaces = MakePopularityGeomPlaces(filtered);
    auto const nameToNode = GetPlacesMap(geomPlaces);
    auto const m = PopularityBuilder::GetAreaMap(geomPlaces);
    auto const tree = PopularityBuilder::MakeTree4d(geomPlaces);
    PopularityBuilder::LinkGeomPlaces(m, tree, geomPlaces);

    TEST_EQUAL(nameToNode.size(), 3, ());
    TEST_EQUAL(nameToNode.at("1")->GetParent(), PopularityBuilder::Node::Ptr(), ());
    TEST_EQUAL(nameToNode.at("1_2")->GetParent(), nameToNode.at("1"), ());
    TEST_EQUAL(nameToNode.at("1_3")->GetParent(), nameToNode.at("1"), ());
  }

  static void MakeNodes()
  {
    std::vector<FeatureBuilder1> v = FilterArea(m_testSet);
    auto const nodes = PopularityBuilder::MakeNodes(v);
    TEST_EQUAL(nodes.size(), v.size(), ());
    for (size_t i = 0; i < v.size(); ++i)
      TEST_EQUAL(nodes[i]->GetData().GetId(), v[i].GetMostGenericOsmId() , ());
  }

  void Build() const
  {
    auto const filename = GetFileName();
    auto const type = m_cl.GetTypeByPath({"tourism", "museum"});
    auto const typeName = m_cl.GetReadableObjectName(type);
    auto const testSet = AddTypes(m_testSet, {type});

    {
      feature::FeaturesCollector collector(filename);
      for (auto const & feature : testSet)
        collector(feature);
    }

    PopularityBuilder builder(filename);
    auto const lines = builder.Build();

    std::map<std::string, PopularityLine> m;
    for (auto const & line : lines)
      m.emplace(line.m_name, line);

    SCOPE_GUARD(RemoveFile, [&] {
      Platform::RemoveFileIfExists(filename);
    });

    TEST_EQUAL(lines.size(), testSet.size(), ());
    TEST(!m.at("1").m_parent, ());
    TEST_EQUAL(m.at("1").m_type,  typeName, ());

    TEST(m.at("1_2").m_parent, ());
    TEST_EQUAL(*m.at("1_2").m_parent, m.at("1").m_id, ());
    TEST_EQUAL(m.at("1_2").m_type, typeName, ());

    TEST(m.at("1_3").m_parent, ());
    TEST_EQUAL(*m.at("1_3").m_parent, m.at("1").m_id, ());
    TEST_EQUAL(m.at("1_3").m_type, typeName, ());

    TEST(m.at("1_3_4").m_parent, ());
    TEST_EQUAL(*m.at("1_3_4").m_parent, m.at("1_3").m_id, ());
    TEST_EQUAL(m.at("1_3_4").m_type, typeName, ());

    TEST(m.at("1_3_5").m_parent, ());
    TEST_EQUAL(*m.at("1_3_5").m_parent, m.at("1_3").m_id, ());
    TEST_EQUAL(m.at("1_3_5").m_type, typeName, ());
  }

private:
  static std::vector<FeatureBuilder1> GetTestSet()
  {
    std::vector<FeatureBuilder1> v;
    v.reserve(5);

    {
      FeatureBuilder1 feature;
      feature.AddOsmId(base::GeoObjectId(1));
      auto const firstLast = m2::PointD{0.0, 0.0};
      feature.AddPoint(firstLast);
      feature.AddPoint(m2::PointD{0.0, 7.0});
      feature.AddPoint(m2::PointD{10.0, 7.0});
      feature.AddPoint(m2::PointD{10.0, 0.0});
      feature.AddPoint(firstLast);
      feature.SetArea();
      feature.AddName("default", "1");
      v.push_back(feature);
    }

    {
      FeatureBuilder1 feature;
      feature.AddOsmId(base::GeoObjectId(2));
      auto const firstLast = m2::PointD{2.0, 3.0};
      feature.AddPoint(firstLast);
      feature.AddPoint(m2::PointD{2.0, 5.0});
      feature.AddPoint(m2::PointD{4.0, 5.0});
      feature.AddPoint(m2::PointD{4.0, 3.0});
      feature.AddPoint(firstLast);
      feature.SetArea();
      feature.AddName("default", "1_2");
      v.push_back(feature);
    }

    {
      FeatureBuilder1 feature;
      feature.AddOsmId(base::GeoObjectId(3));
      auto const firstLast = m2::PointD{6.0, 0.0};
      feature.AddPoint(firstLast);
      feature.AddPoint(m2::PointD{6.0, 3.0});
      feature.AddPoint(m2::PointD{10.0, 3.0});
      feature.AddPoint(m2::PointD{10.0, 0.0});
      feature.AddPoint(firstLast);
      feature.SetArea();
      feature.AddName("default", "1_3");
      v.push_back(feature);
    }

    {
      FeatureBuilder1 feature;
      feature.AddOsmId(base::GeoObjectId(4));
      feature.SetCenter(m2::PointD{8.0, 2.0});
      feature.AddName("default", "1_3_4");
      v.push_back(feature);
    }

    {
      FeatureBuilder1 feature;
      feature.AddOsmId(base::GeoObjectId(5));
      feature.SetCenter(m2::PointD{7.0, 2.0});
      feature.AddName("default", "1_3_5");
      v.push_back(feature);
    }

    return v;
  }

  static std::vector<FeatureBuilder1> FilterArea(std::vector<FeatureBuilder1> const & v)
  {
    std::vector<FeatureBuilder1> filtered;
    std::copy_if(std::begin(v), std::end(v), std::back_inserter(filtered), [](auto const & feature) {
      return feature.IsArea() && feature.IsGeometryClosed();
    });

    return filtered;
  }

  static std::vector<FeatureBuilder1> FilterPoint(std::vector<FeatureBuilder1> const & v)
  {
    std::vector<FeatureBuilder1> filtered;
    std::copy_if(std::begin(v), std::end(v), std::back_inserter(filtered), [](auto const & feature) {
      return feature.IsPoint();
    });

    return filtered;
  }

  static std::map<std::string, PopularityBuilder::Node::Ptr>
  GetPlacesMap(PopularityBuilder::Node::PtrList const & geomPlaces)
  {
    std::map<std::string, PopularityBuilder::Node::Ptr> nameToNode;
    for (auto const & place : geomPlaces)
      nameToNode.emplace(place->GetData().GetFeature().GetName(), place);

    return nameToNode;
  }

  static PopularityBuilder::Node::PtrList MakePopularityGeomPlaces(std::vector<FeatureBuilder1> const & v)
  {
    PopularityBuilder::Node::PtrList nodes;
    nodes.reserve(v.size());
    std::transform(std::begin(v), std::end(v), std::back_inserter(nodes), [](FeatureBuilder1 const & f) {
      return std::make_shared<PopularityBuilder::Node>(PopularityGeomPlace(f));
    });

    return nodes;
  }

  static std::vector<FeatureBuilder1> AddTypes(std::vector<FeatureBuilder1> v, std::vector<uint32_t> const & types)
  {
    for (auto & feature : v)
    {
      for (auto const & type : types)
        feature.AddType(type);
    }

    return v;
  }

  static Classificator & m_cl;
  static std::vector<FeatureBuilder1> const m_testSet;
};

// static
Classificator & TestPopularityBuilder::m_cl = classif();
std::vector<FeatureBuilder1> const TestPopularityBuilder::m_testSet = TestPopularityBuilder::GetTestSet();
}  // namespace generator_tests

using namespace generator_tests;

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_GetType)
{
  TestPopularityBuilder::GetType();
}

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_GetFeatureName)
{
  TestPopularityBuilder::GetFeatureName();
}

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_FindPointParent)
{
  TestPopularityBuilder::FindPointParent();
}

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_FindPopularityGeomPlaceParent)
{
  TestPopularityBuilder::FindPopularityGeomPlaceParent();
}

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_GetAreaMap)
{
  TestPopularityBuilder::GetAreaMap();
}

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_MakeTree4d)
{
  TestPopularityBuilder::MakeTree4d();
}

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_LinkGeomPlaces)
{
  TestPopularityBuilder::LinkGeomPlaces();
}

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_MakeNodes)
{
  TestPopularityBuilder::MakeNodes();
}

UNIT_CLASS_TEST(TestPopularityBuilder, PopularityBuilder_Build)
{
  TestPopularityBuilder::Build();
}
