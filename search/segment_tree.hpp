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
  double static constexpr kNegativeInfinity = -std::numeric_limits<double>::max();
  double static constexpr kPositiveInfinity = std::numeric_limits<double>::max();
  size_t static constexpr kInvalidId = std::numeric_limits<size_t>::max();

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

    bool operator==(Segment const & rhs) const
    {
      return m_id == rhs.m_id && m_from == rhs.m_from && m_to == rhs.m_to;
    }

    // *NOTE* Do not change these values without care - current code
    // depends on default values.
    double m_from = kPositiveInfinity;
    double m_to = kPositiveInfinity;
    size_t m_id = kInvalidId;
  };

  struct Node
  {
    Segment m_segment;
    double m_to = kNegativeInfinity;
    bool m_deleted = true;
  };

  // *NOTE* |segments| must be sorted.
  SegmentTree(std::vector<Segment> const & segments);

  void Add(Segment const & segment);
  void Erase(Segment const & segment);

  // Calls |fn| on any segment containing |x|.
  template <typename Fn>
  void Find(double x, Fn && fn) const
  {
    return Find(0 /* index */, x, fn);
  }

private:
  static size_t LeftChild(size_t index) { return 2 * index + 1; }
  static size_t RightChild(size_t index) { return 2 * index + 2; }

  bool Exists(size_t index) const { return index < m_tree.size(); }

  template <typename Fn>
  void FindSegment(size_t index, Segment const & segment, Fn && fn);

  template <typename Fn>
  void Find(size_t index, double x, Fn & fn) const
  {
    if (!Exists(index))
      return;

    auto const & root = m_tree[index];
    auto const & segment = root.m_segment;

    if (!root.m_deleted && x >= segment.m_from && x <= segment.m_to)
      return fn(segment);

    auto const lc = LeftChild(index);
    if (x < segment.m_from || (Exists(lc) && m_tree[lc].m_to >= x))
      return Find(lc, x, fn);

    return Find(RightChild(index), x, fn);
  }

  void BuildTree(size_t index, std::vector<Segment> const & segments, size_t left, size_t right);

  void Update(size_t index);

  std::vector<Node> m_tree;
};
}  // namespace search
