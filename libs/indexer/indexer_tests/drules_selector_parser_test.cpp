#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_selector.hpp"
#include "indexer/drules_selector_parser.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_object.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace drules_selector_parser_test
{
using generator::tests_support::TestWithClassificator;
using generator::tests_support::TestWithCustomMwms;
using namespace drule;
using namespace std;

UNIT_CLASS_TEST(TestWithCustomMwms, CyclingResearch_SaturatedMapRoundTrip)
{
  auto const & c = classif();
  feature::FeatureBuilder road;
  road.AssignPoints({{0, 0}, {0.01, 0.01}});
  road.SetLinear();
  auto const highway = c.GetTypeByPath({"highway", "tertiary"});
  road.AddType(highway);
  vector<uint32_t> retained{highway};
  for (auto const * tag : {"oneway", "yesbicycle", "bidir_bicycle", "yesfoot", "nocar", "nosidewalk"})
  {
    auto const type = c.GetTypeByPath({"hwtag", tag});
    road.AddType(type);
    retained.push_back(type);
  }
  auto const surface = c.GetTypeByPath({"psurface", "paved_good"});
  auto const lit = c.GetTypeByPath({"hwtag", "lit"});
  retained.push_back(surface);
  road.AddType(surface);
  road.AddType(lit);
  TEST_EQUAL(road.GetParams().FinishAddingTypesEx(), FeatureParams::TYPES_EXCEED_MAX, ());
  TEST(!road.GetParams().IsTypeExist(lit), ());
  auto id = BuildCountry("CyclingResearchRoundTrip",
                         [&](generator::tests_support::TestMwmBuilder & builder) { TEST(builder.Add(road), ()); });
  FeaturesLoaderGuard loader(m_dataSource, id);
  auto feature = loader.GetFeatureByIndex(0);
  TEST(feature, ());
  feature::TypesHolder types(*feature);
  TEST_EQUAL(types.Size(), 8, ());
  for (auto type : retained)
    TEST(types.Has(type), (type));
  auto selector = ParseSelector(vector<string>{"extra_tag=highway=tertiary", "extra_tag=hwtag=yesbicycle",
                                               "extra_tag=hwtag=bidir_bicycle", "extra_tag=psurface=paved_good"});
  TEST(selector, ());
  TEST(selector->Test(*feature, 15), ());
}

UNIT_CLASS_TEST(TestWithClassificator, CyclingResearch_RulesUseTheirOwnClassifier)
{
  Classificator tree;
  istringstream input(
      "world + highway + tertiary - {} natural + coastline - {} mapswithme - "
      "research + a - b - c - d - e - f - lane - {} {}");
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
    Road(Classificator const & c, uint32_t highway, uint32_t lane)
    {
      m_geomType = feature::GeomType::Line;
      m_points = {{0, 0}, {0.01, 0.01}};
      m_types.Add(highway);
      if (lane != 0)
        m_types.Add(lane);
      for (auto const * name : {"a", "b", "c", "d", "e", "f"})
        m_types.Add(c.GetTypeByPath({"research", name}));
    }
  };
  auto tagged = FeatureType::CreateFromMapObject(Road(tree, roadType, laneType));
  auto absent = FeatureType::CreateFromMapObject(Road(tree, roadType, 0));
  TEST_EQUAL(tagged->GetTypesCount(), 8, ());
  auto selected = original;
  feature::FilterRulesByRuntimeSelector(*tagged, 15, holder, selected);
  TEST_EQUAL(selected.size(), 1, ());
  TEST_EQUAL(holder.Find(selected.front())->GetLine()->width, 3.0, ());
  selected = original;
  feature::FilterRulesByRuntimeSelector(*absent, 15, holder, selected);
  TEST_EQUAL(selected.size(), 1, ());
  TEST_EQUAL(holder.Find(selected.front())->GetLine()->width, 1.0, ());
}

UNIT_CLASS_TEST(TestWithClassificator, CyclingResearch_TypeSelectors)
{
  auto const & c = classif();
  FeatureType path(FeatureID{}, c.GetTypeByPath({"highway", "path", "bicycle"}));
  FeatureType road(FeatureID{}, c.GetTypeByPath({"highway", "tertiary"}));
  auto parent = ParseSelector("extra_tag=highway=path", c);
  auto nested = ParseSelector("extra_tag=highway=path=bicycle", c);
  auto absent = ParseSelector("extra_tag!=hwtag=yescar", c);
  TEST(parent && nested && absent, ());
  TEST(parent->Test(path, 15), ());
  TEST(nested->Test(path, 15), ());
  TEST(!parent->Test(road, 15), ());
  TEST(absent->Test(path, 15), ());
  auto conjunction = ParseSelector(vector<string>{"extra_tag=highway=path", "extra_tag!=highway=tertiary"}, c);
  TEST(conjunction, ());
  TEST(conjunction->Test(path, 15), ());
  TEST(!conjunction->Test(road, 15), ());
  for (auto const * invalid : {"extra_tag", "!extra_tag", "extra_tag<highway=path", "extra_tag=does_not_exist",
                               "extra_tag=highway==path", "extra_tag=highway=path="})
    TEST(!ParseSelector(invalid, c), (invalid));
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
  TEST(ParseSelector("population<1000"), ());
  TEST(ParseSelector(vector<string>({"population>1000"})), ());
  TEST(ParseSelector(vector<string>({"population>=1000", "population<=1000000"})), ());
}

UNIT_TEST(NameSelector_Smoke)
{
  TEST(ParseSelector("name"), ());
  TEST(ParseSelector("!name"), ());
}

UNIT_TEST(InvalidSelector_Smoke)
{
  TEST(!ParseSelector(""), ());
  TEST(!ParseSelector(vector<string>({""})), ());
  TEST(!ParseSelector(vector<string>({"population>=1000", "population<=1000000", ""})), ());
}

}  // namespace drules_selector_parser_test
