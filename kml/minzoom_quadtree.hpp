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
// which are greater (in sense of Less) then others on a quadtree level.
// To encode each quadtree level uses two bits. One from ordinate and
// another from abscissa. Both components are mapped to a coordinate
// system that uses the full range of uint32_t inside the bounding (square) box.
template <typename Value, typename Less>
class MinZoomQuadtree
{
public:
  MinZoomQuadtree(Less const & less) : m_less{less} {}

  template <typename ValueType>
  void Add(m2::PointD const & point, ValueType && value) { m_quadtree.push_back({point, std::forward<ValueType>(value)}); }

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

    if (bbox.IsEmptyInterior())
      return;

    auto spacing = std::max(bbox.SizeX() / mercator::Bounds::kRangeX,
                            bbox.SizeY() / mercator::Bounds::kRangeY);
    spacing *= std::sqrt(countPerTile);

    int baseZoom;
    auto const scale = std::frexp(spacing, &baseZoom);
    baseZoom = 1 - baseZoom;

    if (baseZoom >= maxZoom)
      return;

    auto const size = std::max(bbox.SizeX(), bbox.SizeY()) / scale;
    bbox.SetSizes(size, size);

    // Calculate coordinate along z-order curve.
    auto const [minX, minY] = bbox.LeftBottom();
    auto const gridScale = std::numeric_limits<uint32_t>::max() / size;
    for (auto & elem : m_quadtree)
    {
      auto const [x, y] = elem.m_point;
      auto const gridX = static_cast<uint32_t>(std::trunc((x - minX) * gridScale));
      auto const gridY = static_cast<uint32_t>(std::trunc((y - minY) * gridScale));
      // Sacrifice one of 32 possible levels for simplicity of traversing.
      elem.m_zCurveCoord = bits::BitwiseMerge(gridX, gridY) >> 2;
    }

    std::sort(m_quadtree.begin(), m_quadtree.end());

    auto const traverse = [&](auto const & traverse, auto const treeBeg, auto const treeEnd,
                              uint64_t treeLevelMask, int zoom) -> Element *
    {
      if (treeBeg == treeEnd)
        return nullptr;
      if (std::next(treeBeg) == treeEnd)
        return &*treeBeg;
      if (++zoom >= maxZoom)
      {
        auto const rankLess = [&](auto const & lhs, auto const & rhs) -> bool
        {
          return m_less(lhs.m_value, rhs.m_value);
        };
        auto const result = std::max_element(treeBeg, treeEnd, rankLess);
        auto const setMaxZoom = [&](auto & elem) { setMinZoom(elem.m_value, maxZoom); };
        std::for_each(treeBeg, result, setMaxZoom);
        std::for_each(std::next(result), treeEnd, setMaxZoom);
        return &*result;
      }

      Element * topRanked = nullptr;
      treeLevelMask >>= 2;
      CHECK_NOT_EQUAL(treeLevelMask, 0, ());
      uint64_t const quadIncrement = treeLevelMask & (treeLevelMask >> 1);
      uint64_t quadIndex = 0;
      auto const quadMaskEqual = [&](auto const & elem) -> bool
      {
        return (elem.m_zCurveCoord & treeLevelMask) == quadIndex;
      };
      auto quadBeg = treeBeg;
      for (int quadrant = 0; quadrant < 4; ++quadrant)
      {
        ASSERT(std::is_partitioned(quadBeg, treeEnd, quadMaskEqual), ());
        auto const quadEnd = std::partition_point(quadBeg, treeEnd, quadMaskEqual);
        if (auto const elem = traverse(traverse, quadBeg, quadEnd, treeLevelMask, zoom))
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
    auto const rootMask = uint64_t{0b11} << (std::numeric_limits<uint64_t>::digits - 2);
    if (auto const elem = traverse(traverse, m_quadtree.begin(), m_quadtree.end(), rootMask, baseZoom))
      setMinZoom(elem->m_value, 1);
    else
      CHECK(false, (m_quadtree.size()));
  }

private:
  struct Element
  {
    bool operator<(Element const & rhs) const { return m_zCurveCoord < rhs.m_zCurveCoord; }

    m2::PointD m_point;
    Value m_value;
    uint64_t m_zCurveCoord = 0;  // z-order curve coordiante
  };

  Less m_less;
  std::vector<Element> m_quadtree;
};
}  // namespace kml
