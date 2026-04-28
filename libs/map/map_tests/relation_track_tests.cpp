#include "testing/testing.hpp"

#include "map/relation_track.hpp"

#include "coding/point_coding.hpp"

#include "geometry/point_with_altitude.hpp"

#include <initializer_list>

namespace relation_track_tests
{
using namespace geometry;
using Geometry = RelationTrackBuilder::Geometry;

PointWithAltitude MakePt(double x, double y, Altitude alt = kDefaultAltitudeMeters)
{
  return PointWithAltitude({x, y}, alt);
}

// Helper: build a member Geometry from a list of points with an explicit @p relIdx.
// The relation is anchored to a default-constructed MwmId; tests only verify idx order.
Geometry MakeMember(std::initializer_list<PointWithAltitude> pts, uint32_t relIdx)
{
  Geometry g;
  g.m_points = pts;
  g.AddRelationRef({{}, relIdx});
  return g;
}

// Compares @p src's per-relation provenance against @p expected (a list of relation indices).
bool CompareRelsOrder(Geometry const & src, std::initializer_list<uint32_t> const & expected)
{
  if (src.m_relIDs.size() != expected.size())
    return false;
  auto it = expected.begin();
  for (auto const & e : src.m_relIDs)
  {
    if (e.m_index != *it)
      return false;
    ++it;
  }
  return true;
}

UNIT_TEST(BuildChain_SingleMember)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 100), MakePt(1, 0, 200), MakePt(2, 0, 300)}, 1)};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 3, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 100, ());
  TEST_EQUAL(result.m_points[2].GetAltitude(), 300, ());

  TEST(CompareRelsOrder(result, {1}), ());
}

UNIT_TEST(BuildChain_GrowForward)
{
  // Start from member 0, grow forward to member 1.
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)}, 1),
                                   MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40), MakePt(4, 0, 50)}, 2)};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 5, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 10, ());
  TEST_EQUAL(result.m_points[4].GetAltitude(), 50, ());

  TEST(CompareRelsOrder(result, {1, 2}), ());
}

UNIT_TEST(BuildChain_GrowBackward)
{
  // Start from member 1, grow backward to member 0.
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)}, 1),
                                   MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40), MakePt(4, 0, 50)}, 2)};

  auto const result = RelationTrackBuilder::BuildChain(members, 1);
  TEST_EQUAL(result.Size(), 5, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 10, ());
  TEST_EQUAL(result.m_points[4].GetAltitude(), 50, ());

  // Member 1's points are prepended to member 2 (the start). Order = m1 then m2.
  TEST(CompareRelsOrder(result, {1, 2}), ());
}

UNIT_TEST(BuildChain_GrowBothDirections)
{
  // Start from the middle member, grow in both directions.
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 0), MakePt(1, 0, 10)}, 1),
                                   MakeMember({MakePt(1, 0, 10), MakePt(2, 0, 20)}, 2),  // start
                                   MakeMember({MakePt(2, 0, 20), MakePt(3, 0, 30)}, 3)};

  auto const result = RelationTrackBuilder::BuildChain(members, 1);
  TEST_EQUAL(result.Size(), 4, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 0, ());
  TEST_EQUAL(result.m_points[3].GetAltitude(), 30, ());

  TEST(CompareRelsOrder(result, {1, 2, 3}), ());
}

UNIT_TEST(BuildChain_ReversedMember)
{
  // Member 1 is reversed: its back connects to the back of member 0.
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)}, 1),
                                   MakeMember({MakePt(4, 0, 50), MakePt(3, 0, 40), MakePt(2, 0, 30)}, 2)};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 5, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 10, ());
  TEST_EQUAL(result.m_points[3].GetAltitude(), 40, ());
  TEST_EQUAL(result.m_points[4].GetAltitude(), 50, ());

  TEST(CompareRelsOrder(result, {1, 2}), ());
}

UNIT_TEST(BuildChain_DisconnectedMembersPartialChain)
{
  // Member 2 is disconnected — chain should still include members 0 and 1.
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),
                                   MakeMember({MakePt(1, 0, 20), MakePt(2, 0, 30)}, 2),
                                   MakeMember({MakePt(5, 0, 50), MakePt(6, 0, 60)}, 3)};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 3, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 10, ());
  TEST_EQUAL(result.m_points[2].GetAltitude(), 30, ());

  TEST(CompareRelsOrder(result, {1, 2}), ());
}

UNIT_TEST(BuildChain_StartFromMiddleUnordered)
{
  // Members are not in order: 0->2->1, start from 2.
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 0), MakePt(1, 0, 10)}, 1),
                                   MakeMember({MakePt(3, 0, 30), MakePt(4, 0, 40)}, 2),
                                   MakeMember({MakePt(1, 0, 10), MakePt(2, 0, 20), MakePt(3, 0, 30)}, 3)};

  auto const result = RelationTrackBuilder::BuildChain(members, 2);
  TEST_EQUAL(result.Size(), 5, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 0, ());
  TEST_EQUAL(result.m_points[4].GetAltitude(), 40, ());

  // Chain order: m1 prepended, then m3 (start), then m2 appended.
  TEST(CompareRelsOrder(result, {1, 3, 2}), ());
}

UNIT_TEST(BuildChain_AllReversed)
{
  // All members stored in reverse order relative to the chain direction.
  std::vector<Geometry> members = {MakeMember({MakePt(2, 0, 30), MakePt(1, 0, 20), MakePt(0, 0, 10)}, 1),  // start
                                   MakeMember({MakePt(4, 0, 50), MakePt(3, 0, 40), MakePt(2, 0, 30)}, 2),
                                   MakeMember({MakePt(6, 0, 70), MakePt(5, 0, 60), MakePt(4, 0, 50)}, 3)};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 7, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 70, ());
  TEST_EQUAL(result.m_points[6].GetAltitude(), 10, ());

  // Walking the chain from front to back visits each member's points in source order
  // (m3: 4→3→… no, points are (6,5,4) reversed in chain... actually each prepended
  // member m has m.back() == chain.front(), so its forward [m.front()..m[size-2]] is
  // prepended → chain visits each m's points in source order → m_reversed=false.
  TEST(CompareRelsOrder(result, {3, 2, 1}), ());
}

UNIT_TEST(BuildChain_PrefersClosestMemberIdx)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 0), MakePt(1, 0, 10)}, 1),    // 0
                                   MakeMember({MakePt(2, 0, 20), MakePt(3, 0, 30)}, 2),   // 1
                                   MakeMember({MakePt(1, 0, 10), MakePt(2, 0, 20)}, 3),   // 2 (start)
                                   MakeMember({MakePt(3, 0, 30), MakePt(4, 0, 40)}, 4),   // 3
                                   MakeMember({MakePt(2, 0, 20), MakePt(5, 0, 50)}, 5)};  // 4

  auto const result = RelationTrackBuilder::BuildChain(members, 2);
  TEST_EQUAL(result.Size(), 5, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 0, ());
  TEST_EQUAL(result.m_points[4].GetAltitude(), 40, ());

  // Chain: m1 prepended, m3 (start), m2 forward, m4 forward. Member idx 5 stays unused.
  TEST(CompareRelsOrder(result, {1, 3, 2, 4}), ());
}

UNIT_TEST(BuildChain_DuplicateMembers)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 0), MakePt(1, 0, 10)}, 1),    // 0
                                   MakeMember({MakePt(1, 0, 10), MakePt(2, 0, 20)}, 2),   // 1 dup A
                                   MakeMember({MakePt(2, 0, 20), MakePt(3, 0, 30)}, 3),   // 2
                                   MakeMember({MakePt(1, 0, 10), MakePt(2, 0, 20)}, 4)};  // 3 dup B

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 4, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 0, ());
  TEST_EQUAL(result.m_points[3].GetAltitude(), 30, ());

  TEST(CompareRelsOrder(result, {1, 2, 3}), ());
}

UNIT_TEST(BuildChain_NearbyEndpoints)
{
  double const eps = kMwmPointAccuracy * 0.5;
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)}, 1),
                                   MakeMember({MakePt(2 + eps, 0, 30), MakePt(3, 0, 40), MakePt(4, 0, 50)}, 2)};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 5, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 10, ());
  TEST_EQUAL(result.m_points[4].GetAltitude(), 50, ());

  TEST(CompareRelsOrder(result, {1, 2}), ());
}

UNIT_TEST(BuildChain_NearbyEndpointsAtCellBoundary)
{
  double const base = 100 * kMwmPointAccuracy;
  double const a = base - 1e-7;
  double const b = base + 1e-7;
  TEST_LESS(fabs(a - b), kMwmPointAccuracy, ());

  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(a, 0, 20)}, 1),
                                   MakeMember({MakePt(b, 0, 20), MakePt(2, 0, 30)}, 2)};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 3, ());
  TEST_EQUAL(result.m_points[0].GetAltitude(), 10, ());
  TEST_EQUAL(result.m_points[2].GetAltitude(), 30, ());

  TEST(CompareRelsOrder(result, {1, 2}), ());
}

UNIT_TEST(BuildChain_PrefersMatchingEndpointOverNearbyEndpoint)
{
  double const chainEndX = 10.9 * kMwmPointAccuracy;
  double const nearbyFrontX = 9.4 * kMwmPointAccuracy;

  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(chainEndX, 0, 20)}, 1),
                                   MakeMember({MakePt(nearbyFrontX, 0, 30), MakePt(chainEndX, 0, 20)}, 2)};

  auto const result = RelationTrackBuilder::BuildChain(members, 0);
  TEST_EQUAL(result.Size(), 3, ());
  TEST_EQUAL(result.m_points[2].GetAltitude(), 30, ());
  TEST(!result.m_points[1].GetPoint().EqualDxDy(result.m_points[2].GetPoint(), kMwmPointAccuracy), ());

  // m2 has back equal to chain.back, so it's spliced reversed.
  TEST(CompareRelsOrder(result, {1, 2}), ());
}

// MergeAllMembers tests.

UNIT_TEST(MergeAllMembers_SingleConnectedChain)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),
                                   MakeMember({MakePt(1, 0, 20), MakePt(2, 0, 30)}, 2),
                                   MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40)}, 3)};

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 4, ());
  TEST(CompareRelsOrder(result[0], {1, 2, 3}), ());
}

UNIT_TEST(MergeAllMembers_TwoDisconnectedChains)
{
  std::vector<Geometry> members = {
      MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1), MakeMember({MakePt(1, 0, 20), MakePt(2, 0, 30)}, 2),
      MakeMember({MakePt(5, 0, 50), MakePt(6, 0, 60)}, 3), MakeMember({MakePt(6, 0, 60), MakePt(7, 0, 70)}, 4)};

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result[0].Size(), 3, ());
  TEST_EQUAL(result[1].Size(), 3, ());
  TEST(CompareRelsOrder(result[0], {1, 2}), ());
  TEST(CompareRelsOrder(result[1], {3, 4}), ());
}

UNIT_TEST(MergeAllMembers_AllDisconnected)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),
                                   MakeMember({MakePt(3, 0, 30), MakePt(4, 0, 40)}, 2),
                                   MakeMember({MakePt(6, 0, 60), MakePt(7, 0, 70)}, 3)};

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 3, ());
  for (auto const & line : result)
    TEST_EQUAL(line.Size(), 2, ());
  TEST(CompareRelsOrder(result[0], {1}), ());
  TEST(CompareRelsOrder(result[1], {2}), ());
  TEST(CompareRelsOrder(result[2], {3}), ());
}

UNIT_TEST(MergeAllMembers_UnorderedMembers)
{
  std::vector<Geometry> members = {MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40)}, 1),   // 0
                                   MakeMember({MakePt(5, 5, 50), MakePt(6, 5, 60)}, 2),   // 1: separate chain
                                   MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 3),   // 2
                                   MakeMember({MakePt(1, 0, 20), MakePt(2, 0, 30)}, 4)};  // 3

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result[0].Size(), 4, ());
  TEST_EQUAL(result[1].Size(), 2, ());
}

UNIT_TEST(MergeAllMembers_ReordersByProximity)
{
  // Three disconnected chains: A(0→1), B(10→11), C(2→3).
  // A stays first. From A.end(1): nearest is C.front(2) → C forward.
  // From C.end(3): nearest is B.front(10) → B forward.
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),    // A
                                   MakeMember({MakePt(10, 0, 50), MakePt(11, 0, 60)}, 2),  // B (far)
                                   MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40)}, 3)};   // C (near A)

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 3, ());
  // A first (from relation order).
  TEST_EQUAL(result[0].m_points.front().GetAltitude(), 10, ());
  TEST_EQUAL(result[0].m_points.back().GetAltitude(), 20, ());
  // C second (closest to A's end).
  TEST_EQUAL(result[1].m_points.front().GetAltitude(), 30, ());
  TEST_EQUAL(result[1].m_points.back().GetAltitude(), 40, ());
  // B last.
  TEST_EQUAL(result[2].m_points.front().GetAltitude(), 50, ());
  TEST_EQUAL(result[2].m_points.back().GetAltitude(), 60, ());
}

UNIT_TEST(MergeAllMembers_ReversesFirstChain)
{
  std::vector<Geometry> members = {MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40)}, 1),
                                   MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 2)};

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result[0].m_points.front().GetAltitude(), 40, ());
  TEST_EQUAL(result[0].m_points.back().GetAltitude(), 30, ());
  TEST_EQUAL(result[1].m_points.front().GetAltitude(), 20, ());
  TEST_EQUAL(result[1].m_points.back().GetAltitude(), 10, ());

  // Both chains were reversed.
  TEST(CompareRelsOrder(result[0], {1}), ());
  TEST(CompareRelsOrder(result[1], {2}), ());
}

UNIT_TEST(MergeAllMembers_DedupAdjacentRelIDs)
{
  // Two Relations (1, 2) contribute two adjacent ways each. Connected line is
  //   0→1 (R1) | 1→2 (R1) | 2→3 (R2) | 3→4 (R2)
  // Input here is shuffled to verify MergeAllMembers stitches without relying on
  // the natural relation-order: we feed [R2-mid, R1-start, R2-end, R1-mid].
  // Adjacent equal IDs in the resulting chain must collapse: expect {1, 2}.
  std::vector<Geometry> members = {MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40)}, 2),   // R2-mid
                                   MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),   // R1-start
                                   MakeMember({MakePt(3, 0, 40), MakePt(4, 0, 50)}, 2),   // R2-end
                                   MakeMember({MakePt(1, 0, 20), MakePt(2, 0, 30)}, 1)};  // R1-mid

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 5, ());
  TEST_EQUAL(result[0].m_points.front().GetAltitude(), 10, ());
  TEST_EQUAL(result[0].m_points.back().GetAltitude(), 50, ());

  // Adjacent {1,1,2,2} collapses to {1,2}.
  TEST(CompareRelsOrder(result[0], {1, 2}), ());
}

UNIT_TEST(MergeAllMembers_FoldedChainKeepsRelIDOrder)
{
  // Y-shape with R1 appearing on both sides of an R2 stretch:
  //   0→1 (R1) | 1→2 (R1) | 2→3 (R2) | 3→4 (R2) | 4→5 (R1)
  // Input shuffled so the algorithm can't lean on relation-order.
  // Adjacent equal IDs collapse, but the second R1 run is non-adjacent to the
  // first (R2 sits between them) and must be kept: expect {1, 2, 1}.
  std::vector<Geometry> members = {MakeMember({MakePt(3, 0, 40), MakePt(4, 0, 50)}, 2),   // R2-end
                                   MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),   // R1-start
                                   MakeMember({MakePt(4, 0, 50), MakePt(5, 0, 60)}, 1),   // R1-tail
                                   MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40)}, 2),   // R2-start
                                   MakeMember({MakePt(1, 0, 20), MakePt(2, 0, 30)}, 1)};  // R1-mid

  auto const result = RelationTrackBuilder::MergeAllMembers(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 6, ());
  TEST_EQUAL(result[0].m_points.front().GetAltitude(), 10, ());
  TEST_EQUAL(result[0].m_points.back().GetAltitude(), 60, ());

  TEST(CompareRelsOrder(result[0], {1, 2, 1}), ());
}

// MergeOrdered tests.

UNIT_TEST(MergeOrdered_AllConnected)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),
                                   MakeMember({MakePt(1, 0, 20), MakePt(2, 0, 30)}, 2),
                                   MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40)}, 3)};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 4, ());
  TEST_EQUAL(result[0].m_points.front().GetAltitude(), 10, ());
  TEST_EQUAL(result[0].m_points.back().GetAltitude(), 40, ());
  TEST(CompareRelsOrder(result[0], {1, 2, 3}), ());
}

UNIT_TEST(MergeOrdered_GapInMiddle)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),
                                   MakeMember({MakePt(1, 0, 20), MakePt(2, 0, 30)}, 2),
                                   MakeMember({MakePt(5, 0, 50), MakePt(6, 0, 60)}, 3),  // gap
                                   MakeMember({MakePt(6, 0, 60), MakePt(7, 0, 70)}, 4)};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result[0].Size(), 3, ());
  TEST_EQUAL(result[1].Size(), 3, ());
  TEST(CompareRelsOrder(result[0], {1, 2}), ());
  TEST(CompareRelsOrder(result[1], {3, 4}), ());
}

UNIT_TEST(MergeOrdered_ReversedMember)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)}, 1),
                                   MakeMember({MakePt(4, 0, 50), MakePt(3, 0, 40), MakePt(2, 0, 30)}, 2)};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 5, ());
  TEST_EQUAL(result[0].m_points[0].GetAltitude(), 10, ());
  TEST_EQUAL(result[0].m_points[3].GetAltitude(), 40, ());
  TEST_EQUAL(result[0].m_points[4].GetAltitude(), 50, ());
  TEST(CompareRelsOrder(result[0], {1, 2}), ());
}

UNIT_TEST(MergeOrdered_SingleMember)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20), MakePt(2, 0, 30)}, 1)};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 3, ());
  TEST(CompareRelsOrder(result[0], {1}), ());
}

UNIT_TEST(MergeOrdered_AllDisconnected)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),
                                   MakeMember({MakePt(3, 0, 30), MakePt(4, 0, 40)}, 2),
                                   MakeMember({MakePt(6, 0, 60), MakePt(7, 0, 70)}, 3)};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 3, ());
  for (auto const & line : result)
    TEST_EQUAL(line.Size(), 2, ());
}

UNIT_TEST(MergeOrdered_NearbyEndpoints)
{
  double const eps = kMwmPointAccuracy * 0.5;
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 10), MakePt(1, 0, 20)}, 1),
                                   MakeMember({MakePt(1 + eps, 0, 20), MakePt(2, 0, 30)}, 2)};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 3, ());
  TEST(CompareRelsOrder(result[0], {1, 2}), ());
}

UNIT_TEST(MergeOrdered_MixedReversals)
{
  std::vector<Geometry> members = {MakeMember({MakePt(0, 0, 0), MakePt(1, 0, 10)}, 1),
                                   MakeMember({MakePt(3, 0, 30), MakePt(2, 0, 20), MakePt(1, 0, 10)}, 2),
                                   MakeMember({MakePt(3, 0, 30), MakePt(4, 0, 40)}, 3)};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 5, ());
  TEST_EQUAL(result[0].m_points[0].GetAltitude(), 0, ());
  TEST_EQUAL(result[0].m_points[4].GetAltitude(), 40, ());
  // m1 normal, m2 reversed (back joined chain.back), m3 normal.
  TEST(CompareRelsOrder(result[0], {1, 2, 3}), ());
}

UNIT_TEST(MergeOrdered_ReversesFirstMember)
{
  std::vector<Geometry> members = {MakeMember({MakePt(2, 0, 30), MakePt(1, 0, 20)}, 1),
                                   MakeMember({MakePt(2, 0, 30), MakePt(3, 0, 40), MakePt(4, 0, 50)}, 2)};

  auto const result = RelationTrackBuilder::MergeOrdered(members);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].Size(), 4, ());
  TEST_EQUAL(result[0].m_points[0].GetAltitude(), 20, ());
  TEST_EQUAL(result[0].m_points[1].GetAltitude(), 30, ());
  TEST_EQUAL(result[0].m_points[3].GetAltitude(), 50, ());
  TEST(CompareRelsOrder(result[0], {1, 2}), ());
}

}  // namespace relation_track_tests
