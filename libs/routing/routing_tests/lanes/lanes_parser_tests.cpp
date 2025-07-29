#include "testing/testing.hpp"

#include "routing/lanes/lanes_parser.hpp"

namespace routing::turns::lanes::test
{
UNIT_TEST(TestParseLaneWays)
{
  std::vector<std::pair<std::string, LaneWays>> const testData = {
      {";", {LaneWay::None}},
      {"none", {LaneWay::None}},
      {"left", {LaneWay::Left}},
      {"right", {LaneWay::Right}},
      {"sharp_left", {LaneWay::SharpLeft}},
      {"slight_left", {LaneWay::SlightLeft}},
      {"merge_to_right", {LaneWay::MergeToRight}},
      {"merge_to_left", {LaneWay::MergeToLeft}},
      {"slight_right", {LaneWay::SlightRight}},
      {"sharp_right", {LaneWay::SharpRight}},
      {"reverse", {LaneWay::ReverseLeft, LaneWay::ReverseRight}},
      {"next_right", {LaneWay::Right}},
      {"slide_left", {LaneWay::Through}},
      {"slide_right", {LaneWay::Through}},
  };

  for (auto const & [in, out] : testData)
  {
    LanesInfo const result = ParseLanes(in);
    LaneWays const expected = {out};
    TEST_EQUAL(result.front().laneWays, expected, ());
  }
}

UNIT_TEST(TestParseSingleLane)
{
  {
    LanesInfo const result = ParseLanes("through;right");
    LaneWays constexpr expected = {LaneWay::Through, LaneWay::Right};
    TEST_EQUAL(result.front().laneWays, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("through;Right");
    LaneWays constexpr expected = {LaneWay::Through, LaneWay::Right};
    TEST_EQUAL(result.front().laneWays, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("through ;Right");
    LaneWays constexpr expected = {LaneWay::Through, LaneWay::Right};
    TEST_EQUAL(result.front().laneWays, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left;through");
    LaneWays constexpr expected = {LaneWay::Left, LaneWay::Through};
    TEST_EQUAL(result.front().laneWays, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left");
    LaneWays constexpr expected = {LaneWay::Left};
    TEST_EQUAL(result.front().laneWays, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left;");
    LaneWays constexpr expected = {LaneWay::Left, LaneWay::None};
    TEST_EQUAL(result.front().laneWays, expected, ());
  }

  {
    LanesInfo const result = ParseLanes(";");
    LaneWays constexpr expected = {LaneWay::None};
    TEST_EQUAL(result.front().laneWays, expected, ());
  }

  TEST_EQUAL(ParseLanes("SD32kk*887;;").empty(), true, ());
  TEST_EQUAL(ParseLanes("Что-то на кириллице").empty(), true, ());
  TEST_EQUAL(ParseLanes("משהו בעברית").empty(), true, ());
}

UNIT_TEST(TestParseLanes)
{
  {
    LanesInfo const result = ParseLanes("through|through|through|through;right");
    LanesInfo const expected = {
        {{LaneWay::Through}}, {{LaneWay::Through}}, {{LaneWay::Through}}, {{LaneWay::Through, LaneWay::Right}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left|left;through|through|through");
    LanesInfo const expected = {
        {{LaneWay::Left}}, {{LaneWay::Left, LaneWay::Through}}, {{LaneWay::Through}}, {{LaneWay::Through}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left|through|through");
    LanesInfo const expected = {{{LaneWay::Left}}, {{LaneWay::Through}}, {{LaneWay::Through}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left|le  ft|   through|through   |  right");
    LanesInfo const expected = {
        {{LaneWay::Left}}, {{LaneWay::Left}}, {{LaneWay::Through}}, {{LaneWay::Through}}, {{LaneWay::Right}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left|Left|through|througH|right");
    LanesInfo const expected = {
        {{LaneWay::Left}}, {{LaneWay::Left}}, {{LaneWay::Through}}, {{LaneWay::Through}}, {{LaneWay::Right}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left|Left|through|througH|through;right;sharp_rIght");
    LanesInfo const expected = {{{LaneWay::Left}},
                                {{LaneWay::Left}},
                                {{LaneWay::Through}},
                                {{LaneWay::Through}},
                                {{LaneWay::Through, LaneWay::Right, LaneWay::SharpRight}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left |Left|through|througH|right");
    LanesInfo const expected = {
        {{LaneWay::Left}}, {{LaneWay::Left}}, {{LaneWay::Through}}, {{LaneWay::Through}}, {{LaneWay::Right}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("|||||slight_right");
    LanesInfo const expected = {{{LaneWay::None}}, {{LaneWay::None}}, {{LaneWay::None}},
                                {{LaneWay::None}}, {{LaneWay::None}}, {{LaneWay::SlightRight}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("|");
    LanesInfo const expected = {{{LaneWay::None}}, {{LaneWay::None}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes(";|;;;");
    LanesInfo const expected = {{{LaneWay::None}}, {{LaneWay::None}}};
    TEST_EQUAL(result, expected, ());
  }

  {
    LanesInfo const result = ParseLanes("left|Leftt|through|througH|right");
    TEST_EQUAL(result.empty(), true, ());
  }
}
}  // namespace routing::turns::lanes::test
