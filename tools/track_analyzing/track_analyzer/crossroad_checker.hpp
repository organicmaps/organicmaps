#pragma once

#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/segment.hpp"

#include "base/stl_helpers.hpp"

#include <array>
#include <string>

namespace routing
{
class IsCrossroadChecker
{
public:
  enum class Type
  {
    TurnFromSmallerToBigger,
    TurnFromBiggerToSmaller,
    IntersectionWithBig,
    Count
  };

  using CrossroadInfo = std::array<size_t, base::Underlying(Type::Count)>;

  IsCrossroadChecker(IndexGraph & indexGraph, Geometry & geometry) : m_indexGraph(indexGraph), m_geometry(geometry) {}
  /// \brief Compares two segments by their highway type to find if there was a crossroad between them.
  /// Check if current segment is a joint to find and find all intersections with other roads.
  Type operator()(Segment const & current, Segment const & next) const;

  static void MergeCrossroads(Type from, CrossroadInfo & to);
  static void MergeCrossroads(IsCrossroadChecker::CrossroadInfo const & from, IsCrossroadChecker::CrossroadInfo & to);

private:
  IndexGraph & m_indexGraph;
  Geometry & m_geometry;
};

std::string DebugPrint(IsCrossroadChecker::Type type);
}  // namespace routing
