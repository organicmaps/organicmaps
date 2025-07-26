#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace search
{
// SegmentTree that can be used in offline mode, when all possible
// segments that will be added/removed are known in advance.
//
// Complexity for Add(), Erase() and Find() is O(log(n)), where n is a
// total number of segments.
class SegmentTree
{
public:
  auto static constexpr kNegativeInfinity = -std::numeric_limits<double>::max();
  auto static constexpr kPositiveInfinity = std::numeric_limits<double>::max();
  auto static constexpr kInvalidId = std::numeric_limits<size_t>::max();

  struct Segment
  {
    Segment() = default;
    Segment(double from, double to, size_t id) : m_from(from), m_to(to), m_id(id) {}

    bool operator<(Segment const & rhs) const
    {
      if (m_from != rhs.m_from)
        return m_from < rhs.m_from;
      if (m_to != rhs.m_to)
        return m_to < rhs.m_to;
      return m_id < rhs.m_id;
    }

    bool operator==(Segment const & rhs) const { return m_id == rhs.m_id && m_from == rhs.m_from && m_to == rhs.m_to; }

    // *NOTE* Do not change these values without care - current code
    // depends on default values.
    double m_from = kPositiveInfinity;
    double m_to = kPositiveInfinity;
    size_t m_id = kInvalidId;
  };

  struct Node
  {
    // Segment corresponding to the node.
    Segment m_segment;

    // Maximum value among all right bounds of non-deleted segments in
    // the subtree.
    double m_to = kNegativeInfinity;

    // True when corresponding segment is deleted.
    bool m_deleted = true;
  };

  // *NOTE* |segments| must be sorted.
  SegmentTree(std::vector<Segment> const & segments);

  void Add(Segment const & segment);
  void Erase(Segment const & segment);

  // Calls |fn| on an arbitrary segment containing |x|.
  template <typename Fn>
  void FindAny(double x, Fn && fn) const
  {
    FindAny(0 /* index */, x, fn);
  }

  // Calls |fn| on all segments containing |x| making
  // exactly one call per segment.
  template <typename Fn>
  void FindAll(double x, Fn && fn)
  {
    std::vector<Segment const *> interesting;

    auto const collectFn = [&](Segment const & seg) { interesting.push_back(&seg); };

    while (FindAny(0 /* index */, x, collectFn))
      Erase(*interesting.back());

    for (auto const * p : interesting)
    {
      fn(*p);
      Add(*p);
    }
  }

private:
  static size_t LeftChild(size_t index) { return 2 * index + 1; }
  static size_t RightChild(size_t index) { return 2 * index + 2; }

  bool Exists(size_t index) const { return index < m_tree.size(); }

  template <typename Fn>
  void FindSegment(size_t index, Segment const & segment, Fn && fn);

  template <typename Fn>
  bool FindAny(size_t index, double x, Fn & fn) const
  {
    if (!Exists(index))
      return false;

    auto const & root = m_tree[index];
    auto const & segment = root.m_segment;

    if (!root.m_deleted && x >= segment.m_from && x <= segment.m_to)
    {
      fn(segment);
      return true;
    }

    auto const lc = LeftChild(index);
    if (x < segment.m_from || (Exists(lc) && m_tree[lc].m_to >= x))
      return FindAny(lc, x, fn);

    return FindAny(RightChild(index), x, fn);
  }

  void BuildTree(size_t index, std::vector<Segment> const & segments, size_t left, size_t right);

  void Update(size_t index);

  std::vector<Node> m_tree;
};
}  // namespace search
