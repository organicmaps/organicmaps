#include "testing/testing.hpp"

#include "map/relation_track.hpp"

#include "coding/point_coding.hpp"

#include "geometry/point_with_altitude.hpp"

namespace relation_track_tests
{
using namespace geometry;
using Line = RelationTrackBuilder::TrackGeometry;

PointWithAltitude MakePt(double x, double y, Altitude alt = kDefaultAltitudeMeters)
{
  return PointWithAltitude({x, y}, alt);
}

UNIT_TEST(BuildChain_SingleMember)
{
  std::vector<Line> members = {{MakePt(0, 0, 100), MakePt(1, 0, 200), MakePt(2, 0, 300)}};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.size(), 3, ());
  TEST_EQUAL(result[0].GetAltitude(), 100, ());
  TEST_EQUAL(result[2].GetAltitude(), 300, ());
}

UNIT_TEST(BuildChain_GrowForward)
{
  // Start from member 0, grow forward to member 1.
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)},
                               {MakePt(2, 0, 30), MakePt(3, 0, 40), MakePt(4, 0, 50)}};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].GetAltitude(), 10, ());
  TEST_EQUAL(result[4].GetAltitude(), 50, ());
}

UNIT_TEST(BuildChain_GrowBackward)
{
  // Start from member 1, grow backward to member 0.
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)},
                               {MakePt(2, 0, 30), MakePt(3, 0, 40), MakePt(4, 0, 50)}};

  auto const result = RelationTrackBuilder::BuildChain(members, 1);
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].GetAltitude(), 10, ());
  TEST_EQUAL(result[4].GetAltitude(), 50, ());
}

UNIT_TEST(BuildChain_GrowBothDirections)
{
  // Start from the middle member, grow in both directions.
  std::vector<Line> members = {{MakePt(0, 0, 0), MakePt(1, 0, 10)},
                               {MakePt(1, 0, 10), MakePt(2, 0, 20)},  // start
                               {MakePt(2, 0, 20), MakePt(3, 0, 30)}};

  auto const result = RelationTrackBuilder::BuildChain(members, 1);
  TEST_EQUAL(result.size(), 4, ());
  TEST_EQUAL(result[0].GetAltitude(), 0, ());
  TEST_EQUAL(result[3].GetAltitude(), 30, ());
}

UNIT_TEST(BuildChain_ReversedMember)
{
  // Member 1 is reversed: its back connects to the back of member 0.
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)},
                               {MakePt(4, 0, 50), MakePt(3, 0, 40), MakePt(2, 0, 30)}};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].GetAltitude(), 10, ());
  // After reversal of member 1: (2,0,30) -> (3,0,40) -> (4,0,50).
  TEST_EQUAL(result[3].GetAltitude(), 40, ());
  TEST_EQUAL(result[4].GetAltitude(), 50, ());
}

UNIT_TEST(BuildChain_DisconnectedMembersPartialChain)
{
  // Member 2 is disconnected — chain should still include members 0 and 1.
  std::vector<Line> members = {
      {MakePt(0, 0, 10), MakePt(1, 0, 20)},
      {MakePt(1, 0, 20), MakePt(2, 0, 30)},
      {MakePt(5, 0, 50), MakePt(6, 0, 60)}  // Gap.
  };

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.size(), 3, ());
  TEST_EQUAL(result[0].GetAltitude(), 10, ());
  TEST_EQUAL(result[2].GetAltitude(), 30, ());
}

UNIT_TEST(BuildChain_StartFromMiddleUnordered)
{
  // Members are not in order: 0->2->1, start from 2.
  std::vector<Line> members = {
      {MakePt(0, 0, 0), MakePt(1, 0, 10)},
      {MakePt(3, 0, 30), MakePt(4, 0, 40)},
      {MakePt(1, 0, 10), MakePt(2, 0, 20), MakePt(3, 0, 30)}  // start
  };

  auto const result = RelationTrackBuilder::BuildChain(members, 2);
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].GetAltitude(), 0, ());
  TEST_EQUAL(result[4].GetAltitude(), 40, ());
}

UNIT_TEST(BuildChain_AllReversed)
{
  // All members stored in reverse order relative to the chain direction.
  std::vector<Line> members = {{MakePt(2, 0, 30), MakePt(1, 0, 20), MakePt(0, 0, 10)},  // start: reversed
                               {MakePt(4, 0, 50), MakePt(3, 0, 40), MakePt(2, 0, 30)},
                               {MakePt(6, 0, 70), MakePt(5, 0, 60), MakePt(4, 0, 50)}};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  // Chain grows backward from front of member 0 (which is point (2,0)):
  // members 1 and 2 connect to back of chain (point (0,0)) — no, wait.
  // Member 0: front=(2,0), back=(0,0). After start, chain = [(2,0),(1,0),(0,0)].
  // Forward from (0,0): nothing matches.
  // Backward from (2,0): member 1 has back=(2,0), so prepend forward: (4,0),(3,0) before (2,0).
  // Then backward from (4,0): member 2 has back=(4,0), prepend forward: (6,0),(5,0) before (4,0).
  // Result: [(6,0,70),(5,0,60),(4,0,50),(3,0,40),(2,0,30),(1,0,20),(0,0,10)]
  TEST_EQUAL(result.size(), 7, ());
  TEST_EQUAL(result[0].GetAltitude(), 70, ());
  TEST_EQUAL(result[6].GetAltitude(), 10, ());
}

UNIT_TEST(BuildChain_PrefersClosestMemberIdx)
{
  // Start from member 2. Forward from (2,0): members 1 (delta=1) and 4 (delta=2) both have front at (2,0).
  // Should prefer member 1 (closer idx).
  std::vector<Line> members = {{MakePt(0, 0, 0), MakePt(1, 0, 10)},    // 0
                               {MakePt(2, 0, 20), MakePt(3, 0, 30)},   // 1
                               {MakePt(1, 0, 10), MakePt(2, 0, 20)},   // 2 (start)
                               {MakePt(3, 0, 30), MakePt(4, 0, 40)},   // 3
                               {MakePt(2, 0, 20), MakePt(5, 0, 50)}};  // 4 (competing with 1)

  auto const result = RelationTrackBuilder::BuildChain(members, 2);
  // Forward: picks member 1 (delta=1) over 4 (delta=2). Then from (3,0): member 3.
  // Backward: from (1,0): member 0.
  // Chain: 0 → 2(start) → 1 → 3 = [(0,0),(1,0),(2,0),(3,0),(4,0)]
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].GetAltitude(), 0, ());
  TEST_EQUAL(result[4].GetAltitude(), 40, ());
  // Member 4 is unused (member 1 was picked instead).
}

UNIT_TEST(BuildChain_DuplicateMembers)
{
  // Same geometry at indices 1 and 3 (duplicate road segment in a bus route).
  // Start from 0. Forward from (1,0): members 1 and 3 both match. Prefer 1 (closer to 0).
  // Then from member 1's end (2,0): member 2 connects. Then from (3,0): member 3 connects (duplicate).
  std::vector<Line> members = {{MakePt(0, 0, 0), MakePt(1, 0, 10)},    // 0
                               {MakePt(1, 0, 10), MakePt(2, 0, 20)},   // 1 (duplicate A)
                               {MakePt(2, 0, 20), MakePt(3, 0, 30)},   // 2
                               {MakePt(1, 0, 10), MakePt(2, 0, 20)}};  // 3 (duplicate B)

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  // Should use: 0 → 1 (closer idx) → 2. Member 3 has front (1,0) which doesn't connect to chain end (3,0).
  // But member 3's back (2,0) doesn't connect to (3,0) either. So chain is 0→1→2.
  // Then backward from (0,0): nothing connects. Result: 4 points.
  TEST_EQUAL(result.size(), 4, ());
  TEST_EQUAL(result[0].GetAltitude(), 0, ());
  TEST_EQUAL(result[3].GetAltitude(), 30, ());
}

UNIT_TEST(BuildChain_NearbyEndpoints)
{
  // Endpoints differ by less than kMwmPointAccuracy — should still connect.
  double const eps = kMwmPointAccuracy * 0.5;
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)},
                               {MakePt(2 + eps, 0, 30), MakePt(3, 0, 40), MakePt(4, 0, 50)}};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].GetAltitude(), 10, ());
  TEST_EQUAL(result[4].GetAltitude(), 50, ());
}

UNIT_TEST(BuildChain_NearbyEndpointsAtCellBoundary)
{
  // Points that straddle a cell boundary but are within kMwmPointAccuracy.
  // Use coordinates near a multiple of kMwmPointAccuracy to force different cells.
  double const base = 100 * kMwmPointAccuracy;  // exact cell boundary
  double const a = base - 1e-7;
  double const b = base + 1e-7;
  TEST_LESS(fabs(a - b), kMwmPointAccuracy, ());

  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(a, 0, 20)}, {MakePt(b, 0, 20), MakePt(2, 0, 30)}};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.size(), 3, ());
  TEST_EQUAL(result[0].GetAltitude(), 10, ());
  TEST_EQUAL(result[2].GetAltitude(), 30, ());
}

// MergeAllMembers tests.

UNIT_TEST(MergeAllMembers_SingleConnectedChain)
{
  std::vector<Line> members = {
      {MakePt(0, 0, 10), MakePt(1, 0, 20)}, {MakePt(1, 0, 20), MakePt(2, 0, 30)}, {MakePt(2, 0, 30), MakePt(3, 0, 40)}};

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].size(), 4, ());
}

UNIT_TEST(MergeAllMembers_TwoDisconnectedChains)
{
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20)},
                               {MakePt(1, 0, 20), MakePt(2, 0, 30)},
                               {MakePt(5, 0, 50), MakePt(6, 0, 60)},
                               {MakePt(6, 0, 60), MakePt(7, 0, 70)}};

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result[0].size(), 3, ());
  TEST_EQUAL(result[1].size(), 3, ());
}

UNIT_TEST(MergeAllMembers_AllDisconnected)
{
  std::vector<Line> members = {
      {MakePt(0, 0, 10), MakePt(1, 0, 20)}, {MakePt(3, 0, 30), MakePt(4, 0, 40)}, {MakePt(6, 0, 60), MakePt(7, 0, 70)}};

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 3, ());
  for (auto const & line : result)
    TEST_EQUAL(line.size(), 2, ());
}

UNIT_TEST(MergeAllMembers_UnorderedMembers)
{
  // Members not in sequential order but still connectable.
  std::vector<Line> members = {{MakePt(2, 0, 30), MakePt(3, 0, 40)},   // 0
                               {MakePt(5, 5, 50), MakePt(6, 5, 60)},   // 1: separate chain
                               {MakePt(0, 0, 10), MakePt(1, 0, 20)},   // 2
                               {MakePt(1, 0, 20), MakePt(2, 0, 30)}};  // 3

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 2, ());
  // First chain starts from member 0 and grows to include 2 and 3.
  TEST_EQUAL(result[0].size(), 4, ());
  TEST_EQUAL(result[1].size(), 2, ());
}

UNIT_TEST(MergeAllMembers_ReordersByProximity)
{
  // Three disconnected chains: A(0→1), B(10→11), C(2→3).
  // A stays first. A.back(1) closer to C.front(2) than A.front(0) → A not reversed.
  // From A.end(1): nearest is C.front(2) → C forward. From C.end(3): nearest is B.front(10) → B forward.
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20)},    // A
                               {MakePt(10, 0, 50), MakePt(11, 0, 60)},  // B (far)
                               {MakePt(2, 0, 30), MakePt(3, 0, 40)}};   // C (near A)

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 3, ());
  // A first (from relation order).
  TEST_EQUAL(result[0].front().GetAltitude(), 10, ());
  TEST_EQUAL(result[0].back().GetAltitude(), 20, ());
  // C second (closest to A's end).
  TEST_EQUAL(result[1].front().GetAltitude(), 30, ());
  TEST_EQUAL(result[1].back().GetAltitude(), 40, ());
  // B last.
  TEST_EQUAL(result[2].front().GetAltitude(), 50, ());
  TEST_EQUAL(result[2].back().GetAltitude(), 60, ());
}

UNIT_TEST(MergeAllMembers_ReversesFirstChain)
{
  // A(2→3), B(0→1).
  // A.back(3) → nearest B: B.back(1) dist²=4.
  // A.front(2) → nearest B: B.back(1) dist²=1. Closer → reverse A to (3→2).
  // From A.end(2): B.back(1) dist²=1 → reverse B to (1→0).
  std::vector<Line> members = {{MakePt(2, 0, 30), MakePt(3, 0, 40)}, {MakePt(0, 0, 10), MakePt(1, 0, 20)}};

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result[0].front().GetAltitude(), 40, ());
  TEST_EQUAL(result[0].back().GetAltitude(), 30, ());
  TEST_EQUAL(result[1].front().GetAltitude(), 20, ());
  TEST_EQUAL(result[1].back().GetAltitude(), 10, ());
}

// MergeOrdered tests.

UNIT_TEST(MergeOrdered_AllConnected)
{
  std::vector<Line> members = {
      {MakePt(0, 0, 10), MakePt(1, 0, 20)}, {MakePt(1, 0, 20), MakePt(2, 0, 30)}, {MakePt(2, 0, 30), MakePt(3, 0, 40)}};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].size(), 4, ());
  TEST_EQUAL(result[0].front().GetAltitude(), 10, ());
  TEST_EQUAL(result[0].back().GetAltitude(), 40, ());
}

UNIT_TEST(MergeOrdered_GapInMiddle)
{
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20)},
                               {MakePt(1, 0, 20), MakePt(2, 0, 30)},
                               {MakePt(5, 0, 50), MakePt(6, 0, 60)},  // gap
                               {MakePt(6, 0, 60), MakePt(7, 0, 70)}};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result[0].size(), 3, ());
  TEST_EQUAL(result[1].size(), 3, ());
}

UNIT_TEST(MergeOrdered_ReversedMember)
{
  // Member 1 has its back connecting to the chain's back → reversed append.
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)},
                               {MakePt(4, 0, 50), MakePt(3, 0, 40), MakePt(2, 0, 30)}};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].size(), 5, ());
  TEST_EQUAL(result[0][0].GetAltitude(), 10, ());
  TEST_EQUAL(result[0][3].GetAltitude(), 40, ());
  TEST_EQUAL(result[0][4].GetAltitude(), 50, ());
}

UNIT_TEST(MergeOrdered_SingleMember)
{
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)}};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].size(), 3, ());
}

UNIT_TEST(MergeOrdered_AllDisconnected)
{
  std::vector<Line> members = {
      {MakePt(0, 0, 10), MakePt(1, 0, 20)}, {MakePt(3, 0, 30), MakePt(4, 0, 40)}, {MakePt(6, 0, 60), MakePt(7, 0, 70)}};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 3, ());
  for (auto const & line : result)
    TEST_EQUAL(line.size(), 2, ());
}

UNIT_TEST(MergeOrdered_NearbyEndpoints)
{
  // Endpoints within kMwmPointAccuracy should connect.
  double const eps = kMwmPointAccuracy * 0.5;
  std::vector<Line> members = {{MakePt(0, 0, 10), MakePt(1, 0, 20)}, {MakePt(1 + eps, 0, 20), MakePt(2, 0, 30)}};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].size(), 3, ());
}

UNIT_TEST(MergeOrdered_MixedReversals)
{
  // Chain: 0→1→2, member 1 reversed, member 2 normal.
  std::vector<Line> members = {{MakePt(0, 0, 0), MakePt(1, 0, 10)},
                               {MakePt(3, 0, 30), MakePt(2, 0, 20), MakePt(1, 0, 10)},  // reversed
                               {MakePt(3, 0, 30), MakePt(4, 0, 40)}};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].size(), 5, ());
  TEST_EQUAL(result[0][0].GetAltitude(), 0, ());
  TEST_EQUAL(result[0][4].GetAltitude(), 40, ());
}

UNIT_TEST(MergeOrdered_ReversesFirstMember)
{
  // Member 0 (2→1) has front connecting to member 1 (2→3), not back.
  // Should reverse member 0 to (1→2), then append member 1 forward.
  std::vector<Line> members = {{MakePt(2, 0, 30), MakePt(1, 0, 20)},
                               {MakePt(2, 0, 30), MakePt(3, 0, 40), MakePt(4, 0, 50)}};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].size(), 4, ());
  TEST_EQUAL(result[0][0].GetAltitude(), 20, ());
  TEST_EQUAL(result[0][1].GetAltitude(), 30, ());
  TEST_EQUAL(result[0][3].GetAltitude(), 50, ());
}

}  // namespace relation_track_tests
