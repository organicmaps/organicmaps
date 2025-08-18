#include "testing/testing.hpp"

#include "search/segment_tree.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <set>
#include <vector>

namespace segment_tree_tests
{
using namespace search;
using namespace std;

using Segment = SegmentTree::Segment;

auto const kInvalidId = numeric_limits<size_t>::max();

size_t FindAny(SegmentTree const & tree, double x)
{
  size_t id = kInvalidId;
  bool called = false;
  tree.FindAny(x, [&](SegmentTree::Segment const & segment)
  {
    CHECK(!called, ());
    id = segment.m_id;
    called = true;
  });
  return id;
}

set<size_t> FindAll(SegmentTree & tree, double x)
{
  set<size_t> result;
  tree.FindAll(x, [&](SegmentTree::Segment const & segment)
  {
    auto const id = segment.m_id;
    CHECK(result.find(id) == result.end(), ());
    result.insert(id);
  });
  return result;
}

UNIT_TEST(SegmentTree_Smoke)
{
  vector<Segment> const segments;
  SegmentTree tree(segments);
  TEST_EQUAL(kInvalidId, FindAny(tree, -10), ());
  TEST_EQUAL(kInvalidId, FindAny(tree, 0), ());
  TEST_EQUAL(kInvalidId, FindAny(tree, 3.14), ());
  TEST_EQUAL((set<size_t>()), FindAll(tree, 1.0), ());
}

UNIT_TEST(SegmentTree_Simple)
{
  vector<Segment> segments = {Segment(-10000 /* from */, -10000 /* to */, 0 /* id */), Segment(-10, -6, 1),
                              Segment(-7, -3, 2), Segment(-5, -2, 3)};
  CHECK(is_sorted(segments.begin(), segments.end()), ());

  SegmentTree tree(segments);

  TEST_EQUAL(kInvalidId, FindAny(tree, -4), ());

  tree.Add(segments[0]);
  TEST_EQUAL(kInvalidId, FindAny(tree, -7), ());
  TEST_EQUAL(kInvalidId, FindAny(tree, -4), ());
  TEST_EQUAL((set<size_t>{}), FindAll(tree, -7), ());
  TEST_EQUAL((set<size_t>{0}), FindAll(tree, -10000), ());

  tree.Add(segments[1]);
  TEST_EQUAL(1, FindAny(tree, -7), ());
  TEST_EQUAL(kInvalidId, FindAny(tree, -4), ());
  TEST_EQUAL((set<size_t>{1}), FindAll(tree, -7), ());

  tree.Add(segments[3]);
  TEST_EQUAL(1, FindAny(tree, -7), ());
  TEST_EQUAL(3, FindAny(tree, -4), ());
  TEST_EQUAL((set<size_t>{3}), FindAll(tree, -4), ());

  tree.Erase(segments[1]);
  TEST_EQUAL(kInvalidId, FindAny(tree, -7), ());
  TEST_EQUAL(3, FindAny(tree, -4), ());

  tree.Add(segments[1]);
  tree.Add(segments[2]);
  TEST_EQUAL((set<size_t>{1, 2}), FindAll(tree, -6), ());
  TEST_EQUAL((set<size_t>{2}), FindAll(tree, -5.5), ());
  TEST_EQUAL((set<size_t>{2, 3}), FindAll(tree, -5), ());
}
}  // namespace segment_tree_tests
