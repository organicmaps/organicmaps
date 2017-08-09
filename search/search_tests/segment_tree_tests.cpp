#include "testing/testing.hpp"

#include "search/segment_tree.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>
#include <vector>

using namespace search;
using namespace std;

using Segment = SegmentTree::Segment;

namespace
{
auto const kInvalidId = numeric_limits<size_t>::max();

size_t Find(SegmentTree const & tree, double x)
{
  size_t id = kInvalidId;
  bool called = false;
  tree.Find(x, [&](SegmentTree::Segment const & segment) {
    CHECK(!called, ());
    id = segment.m_id;
    called = true;
  });
  return id;
}

UNIT_TEST(SegmentTree_Smoke)
{
  vector<Segment> const segments;
  SegmentTree tree(segments);
  TEST_EQUAL(kInvalidId, Find(tree, -10), ());
  TEST_EQUAL(kInvalidId, Find(tree, 0), ());
  TEST_EQUAL(kInvalidId, Find(tree, 3.14), ());
}

UNIT_TEST(SegmentTree_Simple)
{
  vector<Segment> segments = {Segment(-10000, -10000, 0 /* id */), Segment(-10, -6, 1 /* id */),
                              Segment(-5, -2, 2 /* id */)};
  TEST(is_sorted(segments.begin(), segments.end()), ());

  SegmentTree tree(segments);

  TEST_EQUAL(kInvalidId, Find(tree, -4), ());

  tree.Add(segments[0]);
  TEST_EQUAL(kInvalidId, Find(tree, -7), ());
  TEST_EQUAL(kInvalidId, Find(tree, -4), ());

  tree.Add(segments[1]);
  TEST_EQUAL(1, Find(tree, -7), ());
  TEST_EQUAL(kInvalidId, Find(tree, -4), ());

  tree.Add(segments[2]);
  TEST_EQUAL(1, Find(tree, -7), ());
  TEST_EQUAL(2, Find(tree, -4), ());

  tree.Erase(segments[1]);
  TEST_EQUAL(kInvalidId, Find(tree, -7), ());
  TEST_EQUAL(2, Find(tree, -4), ());
}
}  // namespace
