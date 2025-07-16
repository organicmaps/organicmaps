#pragma once

#include "kml/types.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

namespace kml
{
// Builds linear quadtree of input values using coordinates provided
// then walks through the quadtree elevating minZoom of those values
// which are greater (in sense of Less) than others at the same
// quadtree level.
// To encode each quadtree level uses two bits. One from ordinate and
// another from abscissa. Both components are mapped to a coordinate
// system that uses the full range of uint32_t inside the bounding
// (squared) box.
template <typename Value, typename Less>
class MinZoomQuadtree
{
public:
  MinZoomQuadtree(Less const & less) : m_less{less} {}

  template <typename V>
  void Add(m2::PointD const & point, V && value)
  {
    m_quadtree.emplace_back(point, std::forward<V>(value));
  }

  void Clear() { m_quadtree.clear(); }

  template <typename F>
  void SetMinZoom(double countPerTile, int maxZoom, F setMinZoom /* void (Value & value, int minZoom) */)
  {
    CHECK_GREATER(countPerTile, 0.0, ());
    CHECK_LESS_OR_EQUAL(maxZoom, scales::GetUpperStyleScale(), ());
    CHECK_GREATER_OR_EQUAL(maxZoom, 1, ());

    if (m_quadtree.empty())
      return;

    m2::RectD bbox;
    for (auto const & elem : m_quadtree)
      bbox.Add(elem.m_point);

    if (!(bbox.SizeX() > 0.0 || bbox.SizeY() > 0.0))
      return;

    // `spacing` is an characteristic interval between values on the map as if they spaced
    // uniformly across the map providing required density (i.e. "count per tile area").
    double spacing = std::min(mercator::Bounds::kRangeX / bbox.SizeX(), mercator::Bounds::kRangeY / bbox.SizeY());
    spacing /= std::sqrt(countPerTile);

    // `spacing` value decomposed into a normalized fraction and an integral power of two.
    // Normalized fraction `scale` (in [1, 2)) used to slightly enlarge bounding box.
    // Integral power of two `baseZoom` is biased exponent of inverse value. It has
    // a meaning exactly the same as the other "zoomLevel"s in the project.
    // Scaling (enlarging) the size of bbox by `scale` is required so that one can
    // set arbitrary values for `countPerTile`, not only powers of four.
    int baseZoom;
    double const scale = 2.0 * std::frexp(spacing, &baseZoom);

    auto const setMaxZoom = [&](auto const treeBeg, auto const treeEnd)
    {
      auto const topRanked = std::max_element(
          treeBeg, treeEnd, [&](auto const & lhs, auto const & rhs) { return m_less(lhs.m_value, rhs.m_value); });
      auto const setMaxZoom = [&](auto & elem) { setMinZoom(elem.m_value, maxZoom); };
      std::for_each(treeBeg, topRanked, setMaxZoom);
      std::for_each(std::next(topRanked), treeEnd, setMaxZoom);
      return &*topRanked;
    };
    if (baseZoom >= maxZoom)
    {
      // At least one element is visible from lowest zoom level.
      setMinZoom(setMaxZoom(m_quadtree.begin(), m_quadtree.end())->m_value, 1);
      return;
    }

    double const size = std::max(bbox.SizeX(), bbox.SizeY()) * scale;
    bbox.SetSizes(size, size);

    // Calculate coordinate along z-order curve.
    auto const [minX, minY] = bbox.LeftBottom();
    double const gridScale = std::numeric_limits<uint32_t>::max() / size;
    for (auto & elem : m_quadtree)
    {
      auto const [x, y] = elem.m_point;
      uint32_t const gridX = static_cast<uint32_t>(std::trunc((x - minX) * gridScale));
      uint32_t const gridY = static_cast<uint32_t>(std::trunc((y - minY) * gridScale));
      elem.m_zCurveCoord = bits::BitwiseMerge(gridX, gridY);
    }

    std::sort(m_quadtree.begin(), m_quadtree.end());

    int constexpr depthMax = std::numeric_limits<uint32_t>::digits;
    uint64_t constexpr firstLevelMask = uint64_t{0b11} << ((depthMax - 1) * 2);
    auto const traverse = [&](auto const & traverse, auto const treeBeg, auto const treeEnd, int depth) -> Element *
    {
      if (treeBeg == treeEnd)
        return nullptr;
      if (std::next(treeBeg) == treeEnd)
        return &*treeBeg;
      int const zoom = baseZoom + depth;
      if (zoom >= maxZoom)
        return setMaxZoom(treeBeg, treeEnd);

      Element * topRanked = nullptr;
      CHECK_LESS_OR_EQUAL(depth, depthMax, ("Quadtree is too deep, try to decrease maxZoom", maxZoom));
      uint64_t const treeLevelMask = firstLevelMask >> ((depth - 1) * 2);
      uint64_t const quadIncrement = treeLevelMask & (treeLevelMask >> 1);
      uint64_t quadIndex = 0;
      auto const quadMaskEqual = [&](auto const & elem) -> bool
      { return (elem.m_zCurveCoord & treeLevelMask) == quadIndex; };
      auto quadBeg = treeBeg;
      for (int quadrant = 0; quadrant < 4; ++quadrant)
      {
        ASSERT(std::is_partitioned(quadBeg, treeEnd, quadMaskEqual), ());
        auto const quadEnd = std::partition_point(quadBeg, treeEnd, quadMaskEqual);
        if (auto const elem = traverse(traverse, quadBeg, quadEnd, depth + 1))
        {
          if (topRanked == nullptr)
            topRanked = elem;
          else if (m_less(topRanked->m_value, elem->m_value))
            setMinZoom(std::exchange(topRanked, elem)->m_value, std::max(zoom, 1));
          else
            setMinZoom(elem->m_value, std::max(zoom, 1));
        }
        quadBeg = quadEnd;
        quadIndex += quadIncrement;
      }
      ASSERT(quadBeg == treeEnd, ());
      return topRanked;
    };
    // Root level is not encoded, so start depth from 1
    if (auto const elem = traverse(traverse, m_quadtree.begin(), m_quadtree.end(), 1 /* depth */))
      setMinZoom(elem->m_value, 1 /* zoom */);  // at least one element is visible from lowest zoom level
    else
      CHECK(false, (m_quadtree.size()));
  }

private:
  struct Element
  {
    template <typename V>
    Element(m2::PointD const & point, V && value) : m_point{point}
                                                  , m_value{std::forward<V>(value)}
    {}

    bool operator<(Element const & rhs) const { return m_zCurveCoord < rhs.m_zCurveCoord; }

    m2::PointD m_point;
    Value m_value;
    uint64_t m_zCurveCoord = 0;  // z-order curve coordiante
  };

  Less m_less;
  std::vector<Element> m_quadtree;
};
}  // namespace kml
