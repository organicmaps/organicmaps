#include "testing/testing.hpp"

#include "routing/lanes/lanes_collapse.hpp"

namespace routing::turns::lanes::test
{
namespace
{
LaneInfo Plain(LaneWays ways)
{
  return {ways, LaneWay::None};
}

LaneInfo Recommended(LaneWay way)
{
  return {{way}, way};
}

LaneInfo Collapsed(LaneWays ways, std::uint16_t count)
{
  return {ways, LaneWay::None, count};
}

void Append(LanesInfo & lanes, LaneInfo const & lane, size_t count)
{
  lanes.insert(lanes.end(), count, lane);
}
}  // namespace

UNIT_TEST(CollapseLanes_FittingLanesArePassedThroughVerbatim)
{
  // Identical adjacent lanes must NOT be merged while the junction fits the cap:
  // ordinary junctions keep one entry per physical lane.
  LanesInfo lanes;
  Append(lanes, Plain({LaneWay::Left}), 2);
  Append(lanes, Recommended(LaneWay::Through), 2);
  Append(lanes, Plain({LaneWay::Right}), 2);

  TEST_EQUAL(CollapseLanes(lanes), CollapsedLanes({lanes, false, false}), ());
}

UNIT_TEST(CollapseLanes_TollPlazaCollapsesToExactCounts)
{
  // 50 booths, the two recommended ones in the middle: 20 + 2 + 28 -> 4 entries, no trimming.
  LanesInfo lanes;
  Append(lanes, Plain({LaneWay::None}), 20);
  Append(lanes, Recommended(LaneWay::Through), 2);
  Append(lanes, Plain({LaneWay::None}), 28);

  LanesInfo expected{Collapsed({LaneWay::None}, 20), Recommended(LaneWay::Through), Recommended(LaneWay::Through),
                     Collapsed({LaneWay::None}, 28)};
  TEST_EQUAL(CollapseLanes(lanes), CollapsedLanes({expected, false, false}), ());
}

UNIT_TEST(CollapseLanes_RecommendedLanesSurviveTrimming)
{
  // Recommended lanes on both edges with interior fillers (the case the old Android-side
  // trimming got wrong: it dropped recommended edge lanes while keeping interior fillers).
  LanesInfo lanes;
  Append(lanes, Recommended(LaneWay::Through), 2);
  Append(lanes, Plain({LaneWay::None}), 10);
  Append(lanes, Recommended(LaneWay::Through), 2);

  LanesInfo expected{Recommended(LaneWay::Through), Recommended(LaneWay::Through), Collapsed({LaneWay::None}, 10),
                     Recommended(LaneWay::Through), Recommended(LaneWay::Through)};
  TEST_EQUAL(CollapseLanes(lanes), CollapsedLanes({expected, false, false}), ());
}

UNIT_TEST(CollapseLanes_MixedPlazaTrimsNonRecommendedEdgesFirst)
{
  // Distinct groups exceed the cap even after collapsing: the non-recommended left edge is
  // sacrificed (and flagged), every recommended lane survives.
  LanesInfo lanes;
  Append(lanes, Plain({LaneWay::None}), 10);
  Append(lanes, Plain({LaneWay::Through}), 5);
  Append(lanes, Plain({LaneWay::None}), 5);
  Append(lanes, Recommended(LaneWay::Through), 3);
  Append(lanes, Plain({LaneWay::None}), 10);
  Append(lanes, Plain({LaneWay::Through}), 5);
  Append(lanes, Plain({LaneWay::None}), 12);

  LanesInfo expected{Collapsed({LaneWay::Through}, 5), Collapsed({LaneWay::None}, 5), Recommended(LaneWay::Through),
                     Recommended(LaneWay::Through),    Recommended(LaneWay::Through), Collapsed({LaneWay::None}, 10),
                     Collapsed({LaneWay::Through}, 5), Collapsed({LaneWay::None}, 12)};
  TEST_EQUAL(CollapseLanes(lanes), CollapsedLanes({expected, true, false}), ());
}

UNIT_TEST(CollapseLanes_InteriorNonRecommendedDroppedBeforeRecommendedEdges)
{
  // Both edges recommended, interior entries all distinct (nothing collapses): interior
  // non-recommended entries go first, without edge flags.
  LanesInfo lanes{Recommended(LaneWay::SharpLeft), Plain({LaneWay::Left}),         Plain({LaneWay::SlightLeft}),
                  Plain({LaneWay::Through}),       Plain({LaneWay::SlightRight}),  Plain({LaneWay::Right}),
                  Plain({LaneWay::MergeToLeft}),   Plain({LaneWay::MergeToRight}), Plain({LaneWay::ReverseLeft}),
                  Recommended(LaneWay::SharpRight)};

  LanesInfo expected{Recommended(LaneWay::SharpLeft), Plain({LaneWay::Through}),       Plain({LaneWay::SlightRight}),
                     Plain({LaneWay::Right}),         Plain({LaneWay::MergeToLeft}),   Plain({LaneWay::MergeToRight}),
                     Plain({LaneWay::ReverseLeft}),   Recommended(LaneWay::SharpRight)};
  TEST_EQUAL(CollapseLanes(lanes), CollapsedLanes({expected, false, false}), ());
}

UNIT_TEST(CollapseLanes_AllRecommendedDegenerateTrimsRight)
{
  LanesInfo lanes;
  Append(lanes, Recommended(LaneWay::Through), 50);

  // Recommended lanes never merge; the strip is cut on the right and flagged.
  LanesInfo expected;
  Append(expected, Recommended(LaneWay::Through), kMaxLanesToDisplay);
  TEST_EQUAL(CollapseLanes(lanes), CollapsedLanes({expected, false, true}), ());
}

UNIT_TEST(CollapseLanes_AllNonRecommendedKeepsRightEdge)
{
  // Alternating distinct patterns, nothing recommended: entries are dropped from the left.
  LanesInfo lanes;
  for (size_t i = 0; i < 25; ++i)
  {
    lanes.push_back(Plain({LaneWay::None}));
    lanes.push_back(Plain({LaneWay::Through}));
  }

  auto const result = CollapseLanes(lanes);
  TEST_EQUAL(result.lanes.size(), kMaxLanesToDisplay, ());
  TEST(result.trimmedLeft, ());
  TEST(!result.trimmedRight, ());
  TEST_EQUAL(result.lanes.back(), Plain({LaneWay::Through}), ());
}
}  // namespace routing::turns::lanes::test
