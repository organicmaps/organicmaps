#include "search/segment_tree.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

#include <algorithm>
#include <functional>

namespace search
{
namespace
{
size_t CeilPow2Minus1(size_t n)
{
  size_t s = 0;
  while (s < n)
    s = 2 * s + 1;
  return s;
}
}  // namespace

SegmentTree::SegmentTree(std::vector<Segment> const & segments) : m_tree(CeilPow2Minus1(segments.size()))
{
  ASSERT(is_sorted(segments.begin(), segments.end()), ());
  BuildTree(0 /* index */, segments, 0 /* left */, m_tree.size() /* right */);
}

void SegmentTree::Add(Segment const & segment)
{
  FindSegment(0 /* index */, segment, [](Node & node) { node.m_deleted = false; });
}

void SegmentTree::Erase(Segment const & segment)
{
  FindSegment(0 /* index */, segment, [](Node & node) { node.m_deleted = true; });
}

template <typename Fn>
void SegmentTree::FindSegment(size_t index, Segment const & segment, Fn && fn)
{
  if (!Exists(index))
    return;

  auto & root = m_tree[index];
  if (root.m_segment == segment)
    fn(root);
  else if (segment < root.m_segment)
    FindSegment(LeftChild(index), segment, std::forward<Fn>(fn));
  else
    FindSegment(RightChild(index), segment, std::forward<Fn>(fn));
  Update(index);
}

void SegmentTree::BuildTree(size_t index, std::vector<Segment> const & segments, size_t left, size_t right)
{
  ASSERT_LESS_OR_EQUAL(left, right, ());
  auto const size = right - left;
  ASSERT(bits::IsPow2Minus1(size), (size));

  if (left == right)
    return;

  auto const middle = left + size / 2;
  BuildTree(LeftChild(index), segments, left, middle);
  BuildTree(RightChild(index), segments, middle + 1, right);

  ASSERT_LESS(index, m_tree.size(), ());
  if (middle < segments.size())
    m_tree[index].m_segment = segments[middle];

  Update(index);
}

void SegmentTree::Update(size_t index)
{
  ASSERT_LESS(index, m_tree.size(), ());
  auto & node = m_tree[index];
  node.m_to = node.m_deleted ? kNegativeInfinity : node.m_segment.m_to;

  auto const lc = LeftChild(index);
  auto const rc = RightChild(index);
  for (auto const c : {lc, rc})
  {
    if (!Exists(c))
      continue;
    node.m_to = std::max(node.m_to, m_tree[c].m_to);
  }
}
}  // namespace search
