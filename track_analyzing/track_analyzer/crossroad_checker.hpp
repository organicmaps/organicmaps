#pragma once

#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/segment.hpp"

#include "base/stl_helpers.hpp"

#include <array>

namespace routing
{
class IsCrossroadChecker
{
public:
  enum class Type
  {
    None,
    TurnFromSmallerToBigger,
    TurnFromBiggerToSmaller,
    FromLink,
    ToLink,
    IntersectionWithBig,
    IntersectionWithSmall,
    IntersectionWithLink,
    Count
  };

  using CrossroadInfo = std::array<size_t, base::Underlying(Type::Count)>;

  IsCrossroadChecker(IndexGraph & indexGraph, Geometry & geometry) : m_indexGraph(indexGraph), m_geometry(geometry) {}
  /// \brief Compares two segments by their highway type to find if there was a crossroad between them.
  /// Check if current segment is a joint to find and find all intersections with other roads.
  CrossroadInfo operator()(Segment const & current, Segment const & next) const;

  static void MergeCrossroads(CrossroadInfo const & from, CrossroadInfo & to);

private:
  IndexGraph & m_indexGraph;
  Geometry & m_geometry;
};
}  // namespace routing
