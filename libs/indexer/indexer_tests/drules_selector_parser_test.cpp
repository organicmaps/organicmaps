#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_selector.hpp"
#include "indexer/drules_selector_parser.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_object.hpp"


#include <sstream>
#include <string>
#include <vector>

namespace drules_selector_parser_test
{
using namespace drule;
using namespace std;

UNIT_TEST(ExtraTag_RulesUseTheirOwnClassifier)
{
  Classificator tree;
  istringstream input(
      "world + highway + tertiary - {} natural + coastline - {} mapswithme - "
      "research + lane - {} {}");
  tree.ReadClassificator(input);
  auto const roadType = tree.GetTypeByPath({"highway", "tertiary"});
  auto const laneType = tree.GetTypeByPath({"research", "lane"});
  DrulesFormat format;
  format.variants = {"fixture"};
  format.colors = {{0xff228844}};
  format.ruleCounts[kRuleLines] = 2;
  TypeEntry entry;
  entry.name = "highway-tertiary";
  for (auto const * predicate : {"extra_tag=research=lane", "extra_tag!=research=lane"})
  {
    Element element;
    element.scale = 15;
    element.applyIf = {predicate};
    LineRule line;
    line.width = entry.elements.empty() ? 3.0 : 1.0;
    line.priority = 100;
    element.lines.push_back(line);
    entry.elements.push_back(element);
  }
  format.types.push_back(entry);
  RulesHolder holder;
  holder.LoadFromFormat(format, 0, tree);
  KeysT original;
  tree.GetObject(roadType)->GetSuitable(15, feature::GeomType::Line, original);
  TEST_EQUAL(original.size(), 2, ());

  class Road : public osm::MapObject
  {
  public:
    Road(uint32_t highway, uint32_t lane)
    {
      m_geomType = feature::GeomType::Line;
      m_points = {{0, 0}, {0.01, 0.01}};
      m_types.Add(highway);
      if (lane != 0)
        m_types.Add(lane);
    }
  };
  auto tagged = FeatureType::CreateFromMapObject(Road(roadType, laneType));
  auto absent = FeatureType::CreateFromMapObject(Road(roadType, 0));
  TEST_EQUAL(tagged->GetTypesCount(), 2, ());
  auto selected = original;
  feature::FilterRulesByRuntimeSelector(*tagged, 15, holder, selected);
  TEST_EQUAL(selected.size(), 1, ());
  TEST_EQUAL(holder.Find(selected.front())->GetLine()->width, 3.0, ());
  selected = original;
  feature::FilterRulesByRuntimeSelector(*absent, 15, holder, selected);
  TEST_EQUAL(selected.size(), 1, ());
  TEST_EQUAL(holder.Find(selected.front())->GetLine()->width, 1.0, ());
}

UNIT_TEST(ExtraTag_TypeSelectors)
{
  Classificator c;
  istringstream input("world + highway + path + bicycle - {} tertiary - {} natural + coastline - {} mapswithme - {}");
  c.ReadClassificator(input);
  FeatureType path(FeatureID{}, c.GetTypeByPath({"highway", "path", "bicycle"}));
  FeatureType road(FeatureID{}, c.GetTypeByPath({"highway", "tertiary"}));
  auto parent = ParseSelector(vector<string>{"extra_tag= highway = path "}, c);
  auto nested = ParseSelector(vector<string>{"extra_tag=highway=path=bicycle"}, c);
  TEST(parent && nested, ());
  TEST(parent->Test(path, 15), ());
  TEST(nested->Test(path, 15), ());
  TEST(!parent->Test(road, 15), ());
  FeatureType plainPath(FeatureID{}, c.GetTypeByPath({"highway", "path"}));
  auto conjunction = ParseSelector(vector<string>{"extra_tag=highway=path", "extra_tag!=highway=path=bicycle"}, c);
  TEST(conjunction, ());
  TEST(conjunction->Test(plainPath, 15), ());
  TEST(!conjunction->Test(path, 15), ());
  TEST(!conjunction->Test(road, 15), ());
  for (auto const * invalid : {"extra_tag", "!extra_tag", "extra_tag<highway=path", "extra_tag=does_not_exist",
                               "extra_tag=highway==path", "extra_tag=highway=path="})
    TEST(!ParseSelector(vector<string>{invalid}, c), (invalid));
  TEST(!ParseSelector(vector<string>{"extra_tag=highway=path", "extra_tag=does_not_exist"}, c), ());
}

UNIT_TEST(TestDruleSelectorIsSet)
{
  SelectorExpression e;
  TEST(ParseSelector("name", e), ());

  TEST_EQUAL("name", e.m_tag, ());
  TEST_EQUAL("", e.m_value, ());
  TEST_EQUAL(SelectorOperatorIsSet, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorIsSet2)
{
  SelectorExpression e;
  TEST(ParseSelector("bbox_area", e), ());

  TEST_EQUAL("bbox_area", e.m_tag, ());
  TEST_EQUAL("", e.m_value, ());
  TEST_EQUAL(SelectorOperatorIsSet, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorIsNotSet)
{
  SelectorExpression e;
  TEST(ParseSelector("!name", e), ());

  TEST_EQUAL("name", e.m_tag, ());
  TEST_EQUAL("", e.m_value, ());
  TEST_EQUAL(SelectorOperatorIsNotSet, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorIsNotSet2)
{
  SelectorExpression e;
  TEST(ParseSelector("!bbox_area", e), ());

  TEST_EQUAL("bbox_area", e.m_tag, ());
  TEST_EQUAL("", e.m_value, ());
  TEST_EQUAL(SelectorOperatorIsNotSet, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorEqual)
{
  SelectorExpression e;
  TEST(ParseSelector("population=1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorEqual, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorNotEqual)
{
  SelectorExpression e;
  TEST(ParseSelector("population!=1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorNotEqual, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorLess)
{
  SelectorExpression e;
  TEST(ParseSelector("population<1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorLess, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorLess2)
{
  SelectorExpression e;
  TEST(ParseSelector("bbox_area<1000", e), ());

  TEST_EQUAL("bbox_area", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorLess, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorGreater)
{
  SelectorExpression e;
  TEST(ParseSelector("population>1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorGreater, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorGreater2)
{
  SelectorExpression e;
  TEST(ParseSelector("bbox_area>1000", e), ());

  TEST_EQUAL("bbox_area", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorGreater, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorLessOrEqual)
{
  SelectorExpression e;
  TEST(ParseSelector("population<=1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorLessOrEqual, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorGreaterOrEqual)
{
  SelectorExpression e;
  TEST(ParseSelector("population>=1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorGreaterOrEqual, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorInvalid)
{
  char const * const badFormats[] = {
      "",         "=badformat", "!=badformat",   ">badformat", "<badformat", ">=badformat", "<=badformat",
      "bad$name", "!bad$name",  "bad$name=1000",
  };

  for (auto e : badFormats)
  {
    SelectorExpression expr;
    TEST_EQUAL(false, ParseSelector(e, expr), ("string is", e));
  }
}

UNIT_TEST(PopulationSelector_Smoke)
{
  TEST(ParseSelector(vector<string>{"population<1000"}, classif()), ());
  TEST(ParseSelector(vector<string>({"population>1000"}), classif()), ());
  TEST(ParseSelector(vector<string>({"population>=1000", "population<=1000000"}), classif()), ());
}

UNIT_TEST(NameSelector_Smoke)
{
  TEST(ParseSelector(vector<string>{"name"}, classif()), ());
  TEST(ParseSelector(vector<string>{"!name"}, classif()), ());
}

UNIT_TEST(InvalidSelector_Smoke)
{
  TEST(!ParseSelector({""}, classif()), ());
  TEST(!ParseSelector(vector<string>({"population>=1000", "population<=1000000", ""}), classif()), ());
}

}  // namespace drules_selector_parser_test
