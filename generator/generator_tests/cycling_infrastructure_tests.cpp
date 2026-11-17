#include "testing/testing.hpp"

#include "generator/cycling_infrastructure.hpp"
#include "generator/osm_element.hpp"

#include <initializer_list>
#include <string>
#include <utility>

namespace cycling_infrastructure_tests
{
OsmElement Way(std::initializer_list<std::pair<std::string, std::string>> tags)
{
  OsmElement element;
  element.m_type = OsmElement::EntityType::Way;
  for (auto const & [key, value] : tags)
    element.AddTag(key, value);
  return element;
}

UNIT_TEST(CyclingInfrastructure_ExplicitSidesAndSeparatelyMappedPaths)
{
  auto element = Way({{"cycleway:both", "lane"}, {"cycleway:left", "separate"}, {"cycleway:right:oneway", "-1"}});
  auto sides = generator::ReadCyclingSides(element, false);
  TEST(sides, ());
  TEST(sides->m_left.m_kind == cycling::Kind::None, ());
  TEST(sides->m_right.m_kind == cycling::Kind::Lane, ());
  TEST(sides->m_right.m_direction == cycling::Direction::Backward, ());
  TEST(!generator::ReadCyclingSides(Way({{"cycleway", "lane"}}), false), ());
  sides = generator::ReadCyclingSides(Way({{"cycleway:both", "no"}}), false);
  TEST(sides, ());
  TEST(sides->m_left.m_kind == cycling::Kind::None && sides->m_right.m_kind == cycling::Kind::None, ());
}

UNIT_TEST(CyclingInfrastructure_CategoryAndProtectionPolicy)
{
  using cycling::Kind;
  struct Case
  {
    char const * value;
    char const * lane;
    char const * separation;
    Kind expected;
  };
  for (auto const & item :
       {Case{"track", "", "", Kind::Track}, Case{"lane", "", "", Kind::Lane}, Case{"shared_lane", "", "", Kind::Shared},
        Case{"buffered_lane", "", "", Kind::Lane}, Case{"shoulder", "", "", Kind::Shared},
        Case{"protected_lane", "", "", Kind::Track}, Case{"lane", "advisory", "", Kind::Shared},
        Case{"lane", "exclusive", "kerb", Kind::Track}, Case{"lane", "exclusive", "flex_post", Kind::Track},
        Case{"lane", "exclusive", "studs", Kind::Lane}, Case{"lane", "exclusive", "cone", Kind::Lane}})
  {
    auto const element = Way({{"cycleway:both", item.value},
                              {"cycleway:both:lane", item.lane},
                              {"cycleway:both:separation", item.separation}});
    auto const sides = generator::ReadCyclingSides(element, false);
    TEST(sides, (item.value, item.lane, item.separation));
    TEST(sides->m_left.m_kind == item.expected && sides->m_right.m_kind == item.expected,
         (item.value, item.lane, item.separation));
  }
  auto const modifier = generator::ReadCyclingSides(Way({{"cycleway:both:lane", "exclusive"}}), false);
  TEST(modifier && modifier->m_left.m_kind == Kind::Lane && modifier->m_right.m_kind == Kind::Lane, ());
}

UNIT_TEST(CyclingInfrastructure_DirectionAndStoredGeometryPolicy)
{
  auto const element = Way({{"cycleway:left", "track"},
                            {"cycleway:left:oneway", "yes"},
                            {"cycleway:right", "shared_lane"},
                            {"cycleway:right:oneway", "-1"}});
  auto const normal = generator::ReadCyclingSides(element, false);
  auto reversed = generator::ReadCyclingSides(element, true);
  TEST(normal && reversed, ());
  TEST(normal->m_left.m_direction == cycling::Direction::Forward, ());
  TEST(normal->m_right.m_direction == cycling::Direction::Backward, ());
  TEST(reversed->m_left.m_kind == cycling::Kind::Shared && reversed->m_left.m_direction == cycling::Direction::Forward,
       ());
  TEST(
      reversed->m_right.m_kind == cycling::Kind::Track && reversed->m_right.m_direction == cycling::Direction::Backward,
      ());
  reversed->Reverse();
  TEST_EQUAL(cycling::Encode(*normal), cycling::Encode(*reversed), ());
}
}  // namespace cycling_infrastructure_tests
